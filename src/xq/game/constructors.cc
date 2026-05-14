#include <cstdint>
#include <optional>

#include "include/xq/game.h"
#include "src/xq/game/internal.h"

namespace az::game::xq {

namespace {

using ::az::game::xq::internal::HashBoard;
using ::az::game::xq::internal::Idx;
using ::az::game::xq::internal::kBlackAdvisor;
using ::az::game::xq::internal::kBlackCannon;
using ::az::game::xq::internal::kBlackChariot;
using ::az::game::xq::internal::kBlackElephant;
using ::az::game::xq::internal::kBlackGeneral;
using ::az::game::xq::internal::kBlackHorse;
using ::az::game::xq::internal::kBlackSoldier;
using ::az::game::xq::internal::kNoAction;
using ::az::game::xq::internal::kRedAdvisor;
using ::az::game::xq::internal::kRedCannon;
using ::az::game::xq::internal::kRedChariot;
using ::az::game::xq::internal::kRedElephant;
using ::az::game::xq::internal::kRedGeneral;
using ::az::game::xq::internal::kRedHorse;
using ::az::game::xq::internal::kRedSoldier;
using ::az::game::xq::internal::kUndoUnavailable;

XqB StartingBoard() noexcept {
  XqB board{};
  // Red back rank — row 0
  board[Idx(0, 0)] = kRedChariot;
  board[Idx(0, 1)] = kRedHorse;
  board[Idx(0, 2)] = kRedElephant;
  board[Idx(0, 3)] = kRedAdvisor;
  board[Idx(0, 4)] = kRedGeneral;
  board[Idx(0, 5)] = kRedAdvisor;
  board[Idx(0, 6)] = kRedElephant;
  board[Idx(0, 7)] = kRedHorse;
  board[Idx(0, 8)] = kRedChariot;
  // Red cannons — row 2
  board[Idx(2, 1)] = kRedCannon;
  board[Idx(2, 7)] = kRedCannon;
  // Red soldiers — row 3
  board[Idx(3, 0)] = kRedSoldier;
  board[Idx(3, 2)] = kRedSoldier;
  board[Idx(3, 4)] = kRedSoldier;
  board[Idx(3, 6)] = kRedSoldier;
  board[Idx(3, 8)] = kRedSoldier;
  // Black soldiers — row 6
  board[Idx(6, 0)] = kBlackSoldier;
  board[Idx(6, 2)] = kBlackSoldier;
  board[Idx(6, 4)] = kBlackSoldier;
  board[Idx(6, 6)] = kBlackSoldier;
  board[Idx(6, 8)] = kBlackSoldier;
  // Black cannons — row 7
  board[Idx(7, 1)] = kBlackCannon;
  board[Idx(7, 7)] = kBlackCannon;
  // Black back rank — row 9
  board[Idx(9, 0)] = kBlackChariot;
  board[Idx(9, 1)] = kBlackHorse;
  board[Idx(9, 2)] = kBlackElephant;
  board[Idx(9, 3)] = kBlackAdvisor;
  board[Idx(9, 4)] = kBlackGeneral;
  board[Idx(9, 5)] = kBlackAdvisor;
  board[Idx(9, 6)] = kBlackElephant;
  board[Idx(9, 7)] = kBlackHorse;
  board[Idx(9, 8)] = kBlackChariot;
  return board;
}

}  // namespace

XqGame::XqGame() noexcept
    : board_(StartingBoard()),
      round_(0),
      current_player_(kRed),
      position_hash_(HashBoard(board_, current_player_)) {
  action_history_.fill(kNoAction);
  position_history_[0] = position_hash_;
  position_history_valid_[0] = 1;
}

XqGame::XqGame(XqP starting_player) noexcept
    : board_(StartingBoard()),
      round_(0),
      current_player_(starting_player),
      position_hash_(HashBoard(board_, current_player_)) {
  action_history_.fill(kNoAction);
  position_history_[0] = position_hash_;
  position_history_valid_[0] = 1;
}

XqGame::XqGame(const XqB& board, XqP current_player, uint32_t current_round,
               std::optional<XqA> last_action) noexcept
    : board_(board),
      round_(current_round),
      current_player_(current_player),
      position_hash_(HashBoard(board_, current_player_)) {
  action_history_.fill(kNoAction);
  // Snapshot constructor: seed only the current `position_history_`
  // slot. Augmented snapshots are not driven by MCTS undo loops; older
  // history is unknown and therefore marked invalid. See
  // `memory/game_design_details/repetition.md`.
  if (current_round <= kHistoryCap) {
    position_history_[current_round] = position_hash_;
    position_history_valid_[current_round] = 1;
  }
  if (current_round > 0 && current_round <= kHistoryCap) {
    if (last_action.has_value()) {
      action_history_[current_round - 1] = *last_action;
      apply_undo_log_[current_round - 1] = kUndoUnavailable;
    }
  }
}

}  // namespace az::game::xq
