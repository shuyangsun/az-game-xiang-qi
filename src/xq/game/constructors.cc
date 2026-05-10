#include "include/xq/game.h"

namespace az::game::xq {

// TODO(TASK-GAME-CONSTRUCTOR-IMPL): change, add, or delete constructor
// implementations to match your game's design.

XqGame::XqGame(const XqP& starting_player) noexcept
    : cur_player_{starting_player} {}

}  // namespace az::game::xq
