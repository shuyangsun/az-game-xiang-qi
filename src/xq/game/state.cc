#include "include/xq/game.h"

namespace az::game::xq {

XqB XqGame::CanonicalBoard() const noexcept {
  // TODO(TASK-GAME-STATE-IMPL): implementation

  return board_;
}

bool XqGame::IsOver() const noexcept {
  // TODO(TASK-GAME-STATE-IMPL): implementation. If `kMaxRounds` is set,
  // this must return `true` once `CurrentRound() >= *kMaxRounds`.

  return false;
}

float XqGame::GetScore(const XqP& player) const noexcept {
  // TODO(TASK-GAME-STATE-IMPL): implementation

  return 0.0f;
}

}  // namespace az::game::xq
