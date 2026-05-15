#include <array>
#include <cstddef>
#include <cstdint>

#include "include/xq/game.h"
#include "src/xq/game/internal.h"

namespace az::game::xq {

namespace {

using ::az::game::xq::internal::EmitPseudoLegalMoves;
using ::az::game::xq::internal::IsFlyingGenerals;
using ::az::game::xq::internal::IsInCheck;
using ::az::game::xq::internal::IsNoAction;
using ::az::game::xq::internal::kUndoUnavailable;
using ::az::game::xq::internal::kZobristBlackToMove;
using ::az::game::xq::internal::kZobristPieceKeys;
using ::az::game::xq::internal::ZobristPieceIndex;

// Pack the captured piece into one byte for `apply_undo_log_`. Bit
// layout:
//   bit 7: 1 if captured piece was Black, 0 otherwise.
//   bits 0..6: piece-type magnitude (0 if no capture).
constexpr uint8_t PackCaptured(int8_t code) noexcept {
  if (code == 0) return 0;
  const uint8_t magnitude = static_cast<uint8_t>(code > 0 ? code : -code);
  return static_cast<uint8_t>((code < 0 ? 0x80 : 0) | magnitude);
}

constexpr int8_t UnpackCaptured(uint8_t packed) noexcept {
  if (packed == 0) return 0;
  const int8_t magnitude = static_cast<int8_t>(packed & 0x7F);
  return (packed & 0x80) ? static_cast<int8_t>(-magnitude) : magnitude;
}

}  // namespace

size_t XqGame::ValidActionsInto(
    std::array<XqA, kMaxLegalActions>& out) const noexcept {
  if (IsOver()) return 0;

  // Generate pseudo-legal moves directly into the caller's buffer,
  // then filter in place for own-king safety and Flying General. The
  // compaction is safe because the write index never overtakes the
  // read index.
  size_t pseudo_count = 0;
  EmitPseudoLegalMoves(board_, current_player_, std::span<XqA>(out),
                       pseudo_count);

  XqB scratch = board_;
  size_t write = 0;
  for (size_t i = 0; i < pseudo_count; ++i) {
    const XqA a = out[i];
    const int8_t captured = scratch[a.to];
    const int8_t mover = scratch[a.from];
    scratch[a.to] = mover;
    scratch[a.from] = 0;
    const bool legal =
        !internal::IsInCheck(scratch, current_player_) && !IsFlyingGenerals(scratch);
    scratch[a.from] = mover;
    scratch[a.to] = captured;
    if (legal) out[write++] = a;
  }
  return write;
}

size_t XqGame::PolicyIndex(const XqA& action) const noexcept {
  return static_cast<size_t>(action.from) * kBoardCells +
         static_cast<size_t>(action.to);
}

void XqGame::ApplyActionInPlace(const XqA& action) noexcept {
  const int8_t mover = board_[action.from];
  const int8_t captured = board_[action.to];

  // XOR-out the moving piece on `from` and the captured piece on `to`
  // (no-op if `to` was empty), then XOR-in the moving piece on `to`.
  if (mover != 0) {
    position_hash_ ^= kZobristPieceKeys[action.from][ZobristPieceIndex(mover)];
    position_hash_ ^= kZobristPieceKeys[action.to][ZobristPieceIndex(mover)];
  }
  if (captured != 0) {
    position_hash_ ^= kZobristPieceKeys[action.to][ZobristPieceIndex(captured)];
  }
  // Toggle side-to-move.
  position_hash_ ^= kZobristBlackToMove;

  board_[action.to] = mover;
  board_[action.from] = 0;
  current_player_ = !current_player_;

  if (round_ < kHistoryCap) {
    action_history_[round_] = action;
    apply_undo_log_[round_] = PackCaptured(captured);
  }
  ++round_;
  if (round_ <= kHistoryCap) {
    position_history_[round_] = position_hash_;
    position_history_valid_[round_] = 1;
  }
}

void XqGame::UndoLastAction() noexcept {
  if (round_ == 0) return;
  const uint32_t previous_round = round_ - 1;
  if (previous_round >= kHistoryCap) return;
  const XqA action = action_history_[previous_round];
  if (IsNoAction(action) ||
      apply_undo_log_[previous_round] == kUndoUnavailable) {
    return;
  }

  --round_;
  current_player_ = !current_player_;

  const int8_t captured =
      round_ < kHistoryCap ? UnpackCaptured(apply_undo_log_[round_]) : 0;

  const int8_t mover = board_[action.to];
  board_[action.from] = mover;
  board_[action.to] = captured;

  // Reverse the Zobrist updates.
  position_hash_ ^= kZobristBlackToMove;
  if (captured != 0) {
    position_hash_ ^= kZobristPieceKeys[action.to][ZobristPieceIndex(captured)];
  }
  if (mover != 0) {
    position_hash_ ^= kZobristPieceKeys[action.to][ZobristPieceIndex(mover)];
    position_hash_ ^= kZobristPieceKeys[action.from][ZobristPieceIndex(mover)];
  }
}

}  // namespace az::game::xq
