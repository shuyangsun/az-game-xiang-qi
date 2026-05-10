#include <cstdint>
#include <optional>

#include "include/xq/game.h"

namespace az::game::xq {

const XqB& XqGame::GetBoard() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): return the live board reference.
  return board_;
}

uint32_t XqGame::CurrentRound() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): return the running half-move count.
  return round_;
}

XqP XqGame::CurrentPlayer() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): return the side to move.
  return current_player_;
}

std::optional<XqP> XqGame::LastPlayer() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): nullopt before the first move,
  // otherwise the opposite of the current player.
  if (round_ == 0) {
    return std::nullopt;
  }
  return !current_player_;
}

std::optional<XqA> XqGame::LastAction() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): nullopt before the first move,
  // otherwise the action that produced this position.
  if (round_ == 0) {
    return std::nullopt;
  }
  return action_history_[round_ - 1];
}

}  // namespace az::game::xq
