#include "include/xq/game.h"

namespace az::game::xq {

std::string XqGame::BoardReadableString() const noexcept {
  // TODO(TASK-GAME-STR-IMPL): implementation

  return "\"BoardReadableString()\" is not implemented!";
}

XqResult<XqA> XqGame::ActionFromString(
    std::string_view action_str) const noexcept {
  // TODO(TASK-GAME-STR-IMPL): implementation

  return std::unexpected(XqError::kNotImplemented);
}

std::string XqGame::ActionToString(const XqA& action) const noexcept {
  // TODO(TASK-GAME-STR-IMPL): implementation

  return "\"ActionToString(...)\" is not implemented!";
}

}  // namespace az::game::xq
