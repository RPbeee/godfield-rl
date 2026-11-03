#pragma once
#include "godfield/official_state_c_api.h"
#include "godfield/official_state.hpp"

// Private bridge accessor: callers must provide a live same-library handle.
// Public C clients never depend on the opaque handle's C++ representation.
namespace godfield::official::bridge {
[[nodiscard]] const SnapshotState& snapshot(const gfo_state& state) noexcept;
[[nodiscard]] bool has_active_attack(const gfo_state& state) noexcept;
}
