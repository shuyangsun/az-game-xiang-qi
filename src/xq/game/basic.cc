#include "include/xq/game.h"

namespace az::game::xq {

const XqB& XqGame::GetBoard() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): implementation

  return board_;
}

uint32_t XqGame::CurrentRound() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): implementation

  return round_;
}

XqP XqGame::CurrentPlayer() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): implementation

  return cur_player_;
}

std::optional<XqP> XqGame::LastPlayer() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): implementation

  return last_player_;
}

std::optional<XqA> XqGame::LastAction() const noexcept {
  // TODO(TASK-GAME-BASIC-IMPL): implementation

  return last_action_;
}

}  // namespace az::game::xq
