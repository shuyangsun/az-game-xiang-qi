#ifndef ALPHA_ZERO_GAME_XIANG_QI_SRC_XQ_GAME_INTERNAL_H_
#define ALPHA_ZERO_GAME_XIANG_QI_SRC_XQ_GAME_INTERNAL_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "include/xq/game.h"

namespace az::game::xq::internal {

// ============================================================================
// Piece codes (mirrors `memory/game_design_details/board_encoding.md`).
// ============================================================================

constexpr int8_t kEmpty = 0;

constexpr int8_t kRedGeneral = 1;
constexpr int8_t kRedAdvisor = 2;
constexpr int8_t kRedElephant = 3;
constexpr int8_t kRedHorse = 4;
constexpr int8_t kRedChariot = 5;
constexpr int8_t kRedCannon = 6;
constexpr int8_t kRedSoldier = 7;

constexpr int8_t kBlackGeneral = -1;
constexpr int8_t kBlackAdvisor = -2;
constexpr int8_t kBlackElephant = -3;
constexpr int8_t kBlackHorse = -4;
constexpr int8_t kBlackChariot = -5;
constexpr int8_t kBlackCannon = -6;
constexpr int8_t kBlackSoldier = -7;

constexpr int8_t kNumPieceTypes = 7;
// 14 distinct (color, piece-type) codes for Zobrist indexing.
constexpr int8_t kNumPieceCodes = 14;

// ============================================================================
// Cell coordinate helpers.
// ============================================================================

constexpr uint8_t Idx(uint8_t row, uint8_t col) noexcept {
  return static_cast<uint8_t>(row * kBoardCols + col);
}

constexpr uint8_t Row(uint8_t idx) noexcept {
  return static_cast<uint8_t>(idx / kBoardCols);
}

constexpr uint8_t Col(uint8_t idx) noexcept {
  return static_cast<uint8_t>(idx % kBoardCols);
}

constexpr bool InBounds(int row, int col) noexcept {
  return row >= 0 && row < static_cast<int>(kBoardRows) && col >= 0 &&
         col < static_cast<int>(kBoardCols);
}

constexpr bool InRedPalace(int row, int col) noexcept {
  return row >= 0 && row <= 2 && col >= 3 && col <= 5;
}

constexpr bool InBlackPalace(int row, int col) noexcept {
  return row >= 7 && row <= 9 && col >= 3 && col <= 5;
}

constexpr bool InOwnPalace(int row, int col, XqP player) noexcept {
  return player ? InBlackPalace(row, col) : InRedPalace(row, col);
}

// True if (row) is on the player's own side of the river (Red: 0..4,
// Black: 5..9). Used by Soldier (post-river behavior) and Elephant
// (cannot cross).
constexpr bool OnOwnSide(int row, XqP player) noexcept {
  return player ? row >= 5 : row <= 4;
}

// ============================================================================
// Piece predicates.
// ============================================================================

constexpr bool IsRed(int8_t code) noexcept { return code > 0; }
constexpr bool IsBlack(int8_t code) noexcept { return code < 0; }
constexpr bool IsEmpty(int8_t code) noexcept { return code == 0; }

constexpr int8_t PieceType(int8_t code) noexcept {
  return code > 0 ? code : static_cast<int8_t>(-code);
}

// True if `code` belongs to `player`. `player==kRed` means red pieces.
constexpr bool IsOwnedBy(int8_t code, XqP player) noexcept {
  return player ? IsBlack(code) : IsRed(code);
}

// True if `code` belongs to the opponent of `player`.
constexpr bool IsEnemyOf(int8_t code, XqP player) noexcept {
  return code != 0 && !IsOwnedBy(code, player);
}

// ============================================================================
// Zobrist hashing.
// ============================================================================

// Random keys per (cell, piece-code). The piece-code index is
// `code - 1` for Red and `7 + (-code - 1)` for Black; see
// `ZobristPieceIndex` below.
extern const std::array<std::array<uint64_t, kNumPieceCodes>, kBoardCells>
    kZobristPieceKeys;

// Key XOR'd in when it is Black's turn to move.
extern const uint64_t kZobristBlackToMove;

// Map a non-zero piece code to its Zobrist piece index in `[0, 14)`.
constexpr int ZobristPieceIndex(int8_t code) noexcept {
  if (code > 0) {
    return code - 1;            // Red:   1..7  -> 0..6
  }
  return 7 + (-code - 1);       // Black: -1..-7 -> 7..13
}

// Compute the Zobrist hash of a board snapshot from scratch.
[[nodiscard]] uint64_t HashBoard(const XqB& board, XqP current_player) noexcept;

// ============================================================================
// Move generation primitives. Each function appends pseudo-legal
// moves (not yet filtered for self-check / Flying General) to
// `out[count]` and increments `count`. `from` must hold a piece of
// the appropriate type and color. `out` is sized to at least the
// remaining capacity (typically `XqGame::kMaxLegalActions`); these
// helpers do not bounds-check.
// ============================================================================

void EmitGeneralMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, std::size_t& count) noexcept;
void EmitAdvisorMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, std::size_t& count) noexcept;
void EmitElephantMoves(const XqB& board, uint8_t from, XqP player,
                       std::span<XqA> out, std::size_t& count) noexcept;
void EmitHorseMoves(const XqB& board, uint8_t from, XqP player,
                    std::span<XqA> out, std::size_t& count) noexcept;
void EmitChariotMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, std::size_t& count) noexcept;
void EmitCannonMoves(const XqB& board, uint8_t from, XqP player,
                     std::span<XqA> out, std::size_t& count) noexcept;
void EmitSoldierMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, std::size_t& count) noexcept;

// Generate pseudo-legal moves for every piece belonging to `player`.
// Resets `count` to 0, then writes the generated moves to
// `out[0..count)`. Pseudo-legal = follows piece movement rules; does
// NOT yet check self-check or Flying General.
void EmitPseudoLegalMoves(const XqB& board, XqP player,
                          std::span<XqA> out, std::size_t& count) noexcept;

// ============================================================================
// Position predicates.
// ============================================================================

// Find `player`'s General on the board. Returns `kBoardCells` if not
// present (which means the General has been captured — should never
// happen in a normal game).
[[nodiscard]] uint8_t FindGeneral(const XqB& board, XqP player) noexcept;

// True if the cell `target` would be captured by some pseudo-legal
// move of `attacker_player` on the given board. Used for check
// detection. Considers all piece types and their attack rules
// (cannon screen, horse hobbling, etc.).
[[nodiscard]] bool IsCellAttacked(const XqB& board, uint8_t target,
                                  XqP attacker_player) noexcept;

// True if `player`'s General is in check.
[[nodiscard]] bool IsInCheck(const XqB& board, XqP player) noexcept;

// True if both Generals are on the same file with no pieces between
// them ("Flying General" position — illegal).
[[nodiscard]] bool IsFlyingGenerals(const XqB& board) noexcept;

}  // namespace az::game::xq::internal

#endif  // ALPHA_ZERO_GAME_XIANG_QI_SRC_XQ_GAME_INTERNAL_H_
