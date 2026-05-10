#include "include/xq/game.h"

namespace az::game::xq {

std::vector<XqA> XqGame::ValidActions() const noexcept {
  // TODO(TASK-GAME-ACTION-IMPL): implementation. Must be deterministic in
  // the game state and empty iff `IsOver()` returns `true`.

  return {};
}

std::size_t XqGame::PolicyIndex(const XqA& action) const noexcept {
  // TODO(TASK-GAME-ACTION-IMPL): bijection from action to slot in
  // `[0, kPolicySize)`.

  return 0;
}

void XqGame::ApplyActionInPlace(const XqA& action) noexcept {
  // TODO(TASK-GAME-ACTION-IMPL): apply `action` to this state in place.
  // Update `board_`, `cur_player_`, and any history needed by
  // `UndoLastAction`. Must be allocation-free.

  last_player_ = cur_player_;
  last_action_ = action;
  ++round_;
}

void XqGame::UndoLastAction() noexcept {
  // TODO(TASK-GAME-ACTION-IMPL): reverse the most recent
  // `ApplyActionInPlace`. No-op if there is nothing to undo. Must be
  // allocation-free. The placeholder below only supports a single level of
  // undo; extend the private state if MCTS will need deeper rollouts.

  if (!last_action_.has_value()) {
    return;
  }
  cur_player_ = last_player_.value_or(cur_player_);
  last_action_ = std::nullopt;
  last_player_ = std::nullopt;
  if (round_ > 0) {
    --round_;
  }
}

}  // namespace az::game::xq
