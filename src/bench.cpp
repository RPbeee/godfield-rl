#include "godfield/env.hpp"
#include <charconv>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

struct Stats {
    std::uint64_t decisions = 0, wins[2]{}, truncations = 0, checksum = 0;
};
std::uint64_t number(const char* text) {
    std::uint64_t value = 0;
    const std::string_view s(text);
    const auto [end, err] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (err != std::errc{} || end != s.data() + s.size()) throw std::invalid_argument("invalid integer");
    return value;
}
int main(int argc, char** argv) {
    try {
        if (argc > 4) throw std::invalid_argument("too many arguments");
        const auto games = argc > 1 ? number(argv[1]) : 100000;
        const auto threads = argc > 2 ? number(argv[2]) : 1;
        const auto seed = argc > 3 ? number(argv[3]) : 42;
        if (!games || !threads || threads > 256) throw std::invalid_argument("games>0, threads in [1,256]");
        if (games - 1 > std::numeric_limits<std::uint64_t>::max() - seed)
            throw std::invalid_argument("seed range overflow");
        std::vector<Stats> stats(threads);
        std::vector<std::thread> workers;
        const auto start = std::chrono::steady_clock::now();
        for (std::uint64_t t = 0; t < threads; ++t) workers.emplace_back([&, t] {
            godfield::Env env;
            auto& out = stats[t];
            for (auto g = t; g < games; g += threads) {
                env.reset(seed + g);
                godfield::Rng policy(seed + g + 0xa0761d6478bd642fULL);
                while (!env.done()) {
                    const auto actions = env.legal_actions();
                    env.step(actions.values[policy.bounded(static_cast<std::uint32_t>(actions.count))]);
                }
                const auto& s = env.state();
                out.decisions += s.decisions;
                if (s.terminated) ++out.wins[s.winner];
                if (s.truncated) ++out.truncations;
                out.checksum += (seed + g) ^ (static_cast<std::uint64_t>(s.decisions) << 32)
                    ^ (static_cast<std::uint64_t>(s.players[0].hp) << 16)
                    ^ (static_cast<std::uint64_t>(s.players[1].hp) << 8) ^ (s.winner + 1);
            }
        });
        for (auto& worker : workers) worker.join();
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Stats total;
        for (const auto& x : stats) {
            total.decisions += x.decisions; total.wins[0] += x.wins[0]; total.wins[1] += x.wins[1];
            total.truncations += x.truncations; total.checksum += x.checksum;
        }
        std::cout << "ruleset=" << godfield::ruleset << "\npolicy=uniform-legal\ngames=" << games
            << " threads=" << threads << " seed=" << seed << "\nseconds=" << seconds
            << "\ndecisions=" << total.decisions << "\ndecisions_per_second=" << total.decisions / seconds
            << "\ngames_per_second=" << games / seconds << "\nwins=" << total.wins[0] << ',' << total.wins[1]
            << " truncations=" << total.truncations << "\nchecksum=" << total.checksum << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\nUsage: godfield_bench [games=100000] [threads=1] [seed=42]\n";
        return 1;
    }
}
