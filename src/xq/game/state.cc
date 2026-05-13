#include <array>
#include <cstddef>
#include <span>

#include "include/xq/game.h"
#include "src/xq/game/internal.h"

namespace az::game::xq {

namespace {

using ::az::game::xq::internal::EmitPseudoLegalMoves;
using ::az::game::xq::internal::FindGeneral;
using ::az::game::xq::internal::IsFlyingGenerals;
using ::az::game::xq::internal::IsInCheck;

// True iff the side `player` has at least one fully-legal move. We
// re-implement the legality filter here (rather than calling
// `XqGame::ValidActionsInto`) so termination logic can early-exit on
// the first legal move it finds, rather than enumerating all of them.
bool HasAnyLegalMove(const XqB& board, XqP player) noexcept {
  std::array<XqA, XqGame::kMaxLegalActions> pseudo{};
  size_t pseudo_count = 0;
  EmitPseudoLegalMoves(board, player, std::span<XqA>(pseudo), pseudo_count);
  XqB scratch = board;
  for (size_t i = 0; i < pseudo_count; ++i) {
    const XqA& a = pseudo[i];
    const int8_t captured = scratch[a.to];
    const int8_t mover = scratch[a.from];
    scratch[a.to] = mover;
    scratch[a.from] = 0;
    const bool legal =
        !IsInCheck(scratch, player) && !IsFlyingGenerals(scratch);
    scratch[a.from] = mover;
    scratch[a.to] = captured;
    if (legal) return true;
  }
  return false;
}

}  // namespace

XqB XqGame::CanonicalBoard() const noexcept {
  if (current_player_ == kRed) return board_;
  XqB out{};
  for (uint8_t r = 0; r < kBoardRows; ++r) {
    for (uint8_t c = 0; c < kBoardCols; ++c) {
      const int8_t cell = board_[(kBoardRows - 1 - r) * kBoardCols + c];
      out[r * kBoardCols + c] = (cell != 0) ? static_cast<int8_t>(-cell) : 0;
    }
  }
  return out;
}

bool XqGame::IsOver() const noexcept {
  // Termination priority — see
  // memory/game_rules_details/termination.md:
  //   1. Checkmate / stalemate (no legal move for side-to-move)
  //   2. Threefold repetition
  //   3. Max-rounds engine cap
  //
  // We evaluate (1) first so the game's winner / loser is reported
  // correctly when both checkmate and the move limit (or threefold)
  // hold simultaneously.
  if (FindGeneral(board_, current_player_) >= kBoardCells) return true;
  if (!HasAnyLegalMove(board_, current_player_)) return true;

  // Threefold repetition: count occurrences of the current
  // position_hash_ across `position_history_[0..round_-1]`.
  if (round_ >= 4) {  // At minimum 4 plies before a position can recur.
    int count = 0;
    for (uint32_t i = 0; i < round_; ++i) {
      if (position_history_[i] == position_hash_ && ++count >= 2) {
        // Two prior occurrences + the current state = threefold.
        return true;
      }
    }
  }

  if (kMaxRounds.has_value() && round_ >= *kMaxRounds) return true;
  return false;
}

float XqGame::GetScore(const XqP& player) const noexcept {
  // Resolve in the same priority order as IsOver().
  if (FindGeneral(board_, current_player_) >= kBoardCells) {
    // Side-to-move's General is missing — they lose.
    return (player == current_player_) ? -1.0F : 1.0F;
  }
  if (!HasAnyLegalMove(board_, current_player_)) {
    // Checkmate or stalemate — both are losses for the side to move
    // (Asian rule).
    return (player == current_player_) ? -1.0F : 1.0F;
  }
  // Threefold repetition / max-rounds: draw.
  if (round_ >= 4) {
    int count = 0;
    for (uint32_t i = 0; i < round_; ++i) {
      if (position_history_[i] == position_hash_ && ++count >= 2) {
        return 0.0F;
      }
    }
  }
  if (kMaxRounds.has_value() && round_ >= *kMaxRounds) return 0.0F;
  return 0.0F;
}

}  // namespace az::game::xq
