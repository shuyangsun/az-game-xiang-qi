#ifndef ALPHA_ZERO_GAME_XIANG_QI_TESTS_UNIT_VALID_ACTIONS_H_
#define ALPHA_ZERO_GAME_XIANG_QI_TESTS_UNIT_VALID_ACTIONS_H_

#include <array>
#include <cstddef>
#include <vector>

#include "include/xq/game.h"

namespace az::game::xq {

// Test-only convenience that materializes `XqGame::ValidActionsInto`
// into a heap vector so existing test code can keep treating legal
// moves as a `std::vector<XqA>`. Production code paths use the
// allocation-free buffer-based API directly.
[[nodiscard]] inline std::vector<XqA> ValidActions(
    const XqGame& game) noexcept {
  std::array<XqA, XqGame::kMaxLegalActions> buf{};
  const size_t count = game.ValidActionsInto(buf);
  return std::vector<XqA>(buf.begin(), buf.begin() + count);
}

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_TESTS_UNIT_VALID_ACTIONS_H_
