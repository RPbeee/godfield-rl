#include "godfield/learning.hpp"

#include <charconv>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
namespace learning = godfield::learning;

struct Options {
    learning::TrainConfig config{};
    std::uint64_t episodes = 10000;
    std::uint64_t eval_games = 1000;
    std::uint64_t eval_seed = 1000000000;
    std::filesystem::path out = "runs/baseline.gfl";
    std::optional<std::filesystem::path> resume;
    std::optional<std::filesystem::path> evaluate;
    bool greedy = false, help = false;
    bool config_supplied = false, episodes_supplied = false, out_supplied = false;
};

[[noreturn]] void invalid(std::string_view option, std::string_view reason) {
    throw std::invalid_argument(std::string(option) + ": " + std::string(reason));
}

std::uint64_t integer(std::string_view option, std::string_view value,
                      std::uint64_t limit, bool positive = true) {
    std::uint64_t result = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || end != value.data() + value.size() || (positive && !result) || result > limit)
        invalid(option, positive ? "expected a positive integer within range" : "expected a uint64 integer");
    return result;
}

double real(std::string_view option, std::string_view value) {
    std::istringstream input{std::string(value)};
    input.imbue(std::locale::classic());
    double result = 0;
    if (!(input >> std::noskipws >> result) || !input.eof() || !std::isfinite(result))
        invalid(option, "expected a finite decimal number");
    return result;
}

Options parse(int argc, char** argv) {
    Options options;
    std::set<std::string> seen;
    constexpr auto u64_max = std::numeric_limits<std::uint64_t>::max();
    constexpr auto u32_max = std::numeric_limits<std::uint32_t>::max();
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};
        const auto equal = argument.find('=');
        const auto name = argument.substr(0, equal);
        if (!seen.emplace(name).second) invalid(name, "duplicate option");
        if (name == "--help" || name == "--greedy") {
            if (equal != std::string_view::npos) invalid(name, "does not take a value");
            if (name == "--help") options.help = true;
            else options.greedy = true;
            continue;
        }
        const bool config_option = name == "--seed" || name == "--candidates" || name == "--batch"
            || name == "--snapshot-interval" || name == "--opponent" || name == "--learning-rate"
            || name == "--value-learning-rate" || name == "--entropy" || name == "--gamma"
            || name == "--max-decisions";
        if (!config_option && name != "--episodes" && name != "--resume" && name != "--out"
            && name != "--eval-games" && name != "--eval-seed" && name != "--evaluate")
            invalid(name, "unknown option (see --help)");
        std::string_view value;
        if (equal != std::string_view::npos) value = argument.substr(equal + 1);
        else {
            if (i + 1 >= argc || std::string_view(argv[i + 1]).starts_with("--")) invalid(name, "missing value");
            value = argv[++i];
        }
        if (value.empty()) invalid(name, "missing value");
        options.config_supplied |= config_option;
        if (name == "--episodes") {
            options.episodes = integer(name, value, u64_max);
            options.episodes_supplied = true;
        } else if (name == "--seed") options.config.seed = integer(name, value, u64_max, false);
        else if (name == "--candidates") options.config.candidates = static_cast<std::uint32_t>(integer(name, value, u32_max));
        else if (name == "--batch") options.config.batch_size = static_cast<std::uint32_t>(integer(name, value, u32_max));
        else if (name == "--snapshot-interval") options.config.snapshot_interval = static_cast<std::uint32_t>(integer(name, value, u32_max));
        else if (name == "--max-decisions") options.config.max_decisions = static_cast<std::uint32_t>(integer(name, value, u32_max));
        else if (name == "--learning-rate") options.config.learning_rate = real(name, value);
        else if (name == "--value-learning-rate") options.config.value_learning_rate = real(name, value);
        else if (name == "--entropy") options.config.entropy = real(name, value);
        else if (name == "--gamma") options.config.gamma = real(name, value);
        else if (name == "--opponent") {
            if (value == "random") options.config.opponent = learning::Opponent::random;
            else if (value == "snapshot") options.config.opponent = learning::Opponent::snapshot;
            else invalid(name, "expected random or snapshot");
        } else if (name == "--resume") options.resume = std::filesystem::path(value);
        else if (name == "--evaluate") options.evaluate = std::filesystem::path(value);
        else if (name == "--out") { options.out = std::filesystem::path(value); options.out_supplied = true; }
        else if (name == "--eval-games") options.eval_games = integer(name, value, u64_max);
        else if (name == "--eval-seed") options.eval_seed = integer(name, value, u64_max, false);
    }
    if (options.help) return options;
    if (options.resume && options.evaluate) invalid("--evaluate", "cannot be combined with --resume");
    if ((options.resume || options.evaluate) && options.config_supplied)
        invalid(options.resume ? "--resume" : "--evaluate", "uses the saved config; config overrides are not allowed");
    if (options.evaluate && (options.episodes_supplied || options.out_supplied))
        invalid("--evaluate", "evaluation only; cannot be combined with --episodes or --out");
    return options;
}

void help() {
    std::cout <<
        "Usage: godfield_train [options]\n"
        "       godfield_train --resume PATH [--episodes N] [--out PATH] [evaluation options]\n"
        "       godfield_train --evaluate PATH [evaluation options]\n\n"
        "Training:\n"
        "  --episodes N             Episodes to add (default 10000, including resume)\n"
        "  --seed N                 Initial training seed (default 42; zero is valid)\n"
        "  --candidates N           Sampled action candidates (default 12, maximum 32)\n"
        "  --batch N                Episodes per update (default 64)\n"
        "  --snapshot-interval N    Episodes between opponent snapshots (default 1024)\n"
        "  --opponent MODE          random or snapshot (default snapshot)\n"
        "  --learning-rate X        Policy learning rate (default 0.01)\n"
        "  --value-learning-rate X  Value learning rate (default 0.01)\n"
        "  --entropy X              Entropy coefficient (default 0.01)\n"
        "  --gamma X                Discount factor (default 1)\n"
        "  --max-decisions N        Environment decision limit (default 512)\n"
        "  --resume PATH            Load complete training state and saved config\n"
        "  --out PATH               Checkpoint output (default runs/baseline.gfl)\n\n"
        "Evaluation:\n"
        "  --eval-games N           Evaluation games (default 1000)\n"
        "  --eval-seed N            Evaluation seed (default 1000000000; zero is valid)\n"
        "  --greedy                 Select the highest-probability evaluation action\n"
        "  --evaluate PATH          Evaluate only; never write or train the checkpoint\n"
        "  --help                   Show this help\n\n"
        "Counts must be positive. Resume/evaluate reject explicit config overrides.\n"
        "Train/resume evaluate before and after training with the same evaluation seed.\n"
        "Output uses stable key=value lines. Both --option VALUE and --option=VALUE work.\n";
}

std::string printable_path(const std::filesystem::path& path) {
    std::string output;
    for (const char character : path.generic_string()) {
        switch (character) {
        case '\\': output += "\\\\"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default: output += character;
        }
    }
    return output;
}

void print_config(const learning::TrainConfig& config) {
    std::cout << "config.seed=" << config.seed
        << "\nconfig.candidates=" << config.candidates
        << "\nconfig.batch=" << config.batch_size
        << "\nconfig.snapshot_interval=" << config.snapshot_interval
        << "\nconfig.opponent=" << (config.opponent == learning::Opponent::random ? "random" : "snapshot")
        << "\nconfig.learning_rate=" << config.learning_rate
        << "\nconfig.value_learning_rate=" << config.value_learning_rate
        << "\nconfig.entropy=" << config.entropy
        << "\nconfig.gamma=" << config.gamma
        << "\nconfig.clip_norm=" << config.clip_norm
        << "\nconfig.max_decisions=" << config.max_decisions << '\n';
}

void print_evaluation(std::string_view prefix, const learning::Evaluation& result) {
    std::cout << prefix << ".games=" << result.games << '\n'
        << prefix << ".wins=" << result.wins << '\n'
        << prefix << ".losses=" << result.losses << '\n'
        << prefix << ".draws=" << result.draws << '\n'
        << prefix << ".truncations=" << result.truncations << '\n'
        << prefix << ".decisions=" << result.decisions << '\n'
        << prefix << ".win_rate=" << result.win_rate() << '\n'
        << prefix << ".checksum=" << result.checksum << '\n';
}

void print_stats(const learning::Checkpoint& state) {
    std::cout << "stats.episodes=" << state.episodes
        << "\nstats.updates=" << state.updates
        << "\nstats.optimizer_steps=" << state.optimizer_steps
        << "\nstats.wins=" << state.wins
        << "\nstats.losses=" << state.losses
        << "\nstats.draws=" << state.draws
        << "\nstats.truncations=" << state.truncations
        << "\nstats.decisions=" << state.decisions
        << "\nstats.learned_decisions=" << state.learned_decisions
        << "\nstats.pending_episodes=" << state.pending_episodes
        << "\nstats.pending_decisions=" << state.pending_decisions
        << "\nstats.checksum=" << state.checksum << '\n';
}
} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse(argc, argv);
        std::cout.imbue(std::locale::classic());
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);
        if (options.help) { help(); return 0; }
        constexpr auto u64_max = std::numeric_limits<std::uint64_t>::max();
        if (options.eval_seed > u64_max - (options.eval_games - 1))
            invalid("--eval-seed", "evaluation seed range would overflow uint64");
        auto state = options.resume ? learning::load_checkpoint(*options.resume)
            : options.evaluate ? learning::load_checkpoint(*options.evaluate) : learning::initialize(options.config);
        learning::validate_config(state.config);
        if (!options.evaluate) {
            if (options.episodes > u64_max - state.episodes)
                invalid("--episodes", "cumulative episode count would overflow uint64");
            if (state.config.seed > u64_max - state.episodes - options.episodes)
                invalid("--episodes", "training seed range would overflow uint64");
        }
        std::cout << "mode=" << (options.evaluate ? "evaluate" : "train")
            << "\nruleset=" << godfield::legacy::dying_weapon_ruleset
            << "\ncheckpoint.version=" << learning::checkpoint_version
            << "\npolicy.features=" << learning::policy_feature_count
            << "\nvalue.features=" << learning::value_feature_count << '\n';
        print_config(state.config);
        std::cout << "eval.games_requested=" << options.eval_games
            << "\neval.seed=" << options.eval_seed
            << "\neval.greedy=" << static_cast<int>(options.greedy) << '\n';
        if (options.evaluate) {
            print_evaluation("evaluation", learning::evaluate(state, options.eval_games, options.eval_seed, options.greedy));
            print_stats(state);
            std::cout << "checkpoint.path=" << printable_path(*options.evaluate) << '\n';
            return 0;
        }
        std::cout << "episodes.requested=" << options.episodes << '\n';
        print_evaluation("initial", learning::evaluate(state, options.eval_games, options.eval_seed, options.greedy));
        std::cout.flush();
        learning::train(state, options.episodes);
        const auto final = learning::evaluate(state, options.eval_games, options.eval_seed, options.greedy);
        if (options.out.has_parent_path()) std::filesystem::create_directories(options.out.parent_path());
        learning::save_checkpoint(state, options.out);
        print_evaluation("final", final);
        print_stats(state);
        std::cout << "checkpoint.path=" << printable_path(options.out) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "error: unknown failure\n";
        return 1;
    }
}
