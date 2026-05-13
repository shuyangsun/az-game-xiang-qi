#include "src/xq/game/internal.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "include/xq/game.h"

namespace az::game::xq::internal {

namespace {

// Deterministic xorshift64-based PRNG used to seed the Zobrist key
// table. Same seed every run, no platform variance.
struct Xorshift64 {
  uint64_t state;
  constexpr uint64_t Next() noexcept {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }
};

// Build the Zobrist piece-key table at compile time.
constexpr auto MakeZobristPieceKeys() noexcept {
  std::array<std::array<uint64_t, kNumPieceCodes>, kBoardCells> keys{};
  Xorshift64 prng{0x9E3779B97F4A7C15ULL};
  for (auto& cell : keys) {
    for (auto& key : cell) {
      key = prng.Next();
    }
  }
  return keys;
}

constexpr uint64_t MakeZobristBlackKey() noexcept {
  Xorshift64 prng{0xBF58476D1CE4E5B9ULL};
  return prng.Next();
}

// ----------------------------------------------------------------------------
// Move-generation utilities.
// ----------------------------------------------------------------------------

// Push (from, to) if the destination is in bounds and not own piece.
inline void TryPushMove(const XqB& board, uint8_t from, int to_row, int to_col,
                        XqP player, std::span<XqA> out,
                        size_t& count) noexcept {
  if (!InBounds(to_row, to_col)) return;
  const uint8_t to =
      Idx(static_cast<uint8_t>(to_row), static_cast<uint8_t>(to_col));
  if (IsOwnedBy(board[to], player)) return;
  out[count++] = XqA{from, to};
}

// As above but with palace constraint.
inline void TryPushPalaceMove(const XqB& board, uint8_t from, int to_row,
                              int to_col, XqP player, std::span<XqA> out,
                              size_t& count) noexcept {
  if (!InOwnPalace(to_row, to_col, player)) return;
  if (IsOwnedBy(board[Idx(static_cast<uint8_t>(to_row),
                          static_cast<uint8_t>(to_col))],
                player)) {
    return;
  }
  out[count++] = XqA{
      from, Idx(static_cast<uint8_t>(to_row), static_cast<uint8_t>(to_col))};
}

// ============================================================================
// Attack queries by piece type. These are called in the inner loop of
// `IsCellAttacked` for every potentially-attacking piece. They check
// "can a piece of type T at `from` capture target?" rather than
// generating all moves, so they are cheaper than a full move
// generation.
// ============================================================================

bool GeneralAttacks(const XqB& /*board*/, uint8_t from,
                    uint8_t target) noexcept {
  const int fr = Row(from), fc = Col(from);
  const int tr = Row(target), tc = Col(target);
  // Adjacent orthogonal step within the General's palace. We don't
  // enforce the palace constraint here because the caller already
  // knows `from` holds a General — the palace constraint is on the
  // mover, not on the target.
  return (fr == tr && std::abs(fc - tc) == 1) ||
         (fc == tc && std::abs(fr - tr) == 1);
}

bool AdvisorAttacks(const XqB& /*board*/, uint8_t from,
                    uint8_t target) noexcept {
  const int fr = Row(from), fc = Col(from);
  const int tr = Row(target), tc = Col(target);
  return std::abs(fr - tr) == 1 && std::abs(fc - tc) == 1;
}

bool ElephantAttacks(const XqB& board, uint8_t from, uint8_t target,
                     XqP player) noexcept {
  const int fr = Row(from), fc = Col(from);
  const int tr = Row(target), tc = Col(target);
  if (std::abs(fr - tr) != 2 || std::abs(fc - tc) != 2) return false;
  // Cannot cross the river.
  if (!OnOwnSide(tr, player)) return false;
  // Eye must be empty.
  const int er = (fr + tr) / 2;
  const int ec = (fc + tc) / 2;
  return board[Idx(static_cast<uint8_t>(er), static_cast<uint8_t>(ec))] ==
         kEmpty;
}

bool HorseAttacks(const XqB& board, uint8_t from, uint8_t target) noexcept {
  const int fr = Row(from), fc = Col(from);
  const int tr = Row(target), tc = Col(target);
  const int dr = tr - fr;
  const int dc = tc - fc;
  // L-shape: (±2, ±1) or (±1, ±2)
  int leg_r = fr;
  int leg_c = fc;
  if (std::abs(dr) == 2 && std::abs(dc) == 1) {
    leg_r += dr / 2;
  } else if (std::abs(dr) == 1 && std::abs(dc) == 2) {
    leg_c += dc / 2;
  } else {
    return false;
  }
  return board[Idx(static_cast<uint8_t>(leg_r), static_cast<uint8_t>(leg_c))] ==
         kEmpty;
}

bool ChariotAttacks(const XqB& board, uint8_t from, uint8_t target) noexcept {
  const int fr = Row(from), fc = Col(from);
  const int tr = Row(target), tc = Col(target);
  if (fr != tr && fc != tc) return false;
  if (from == target) return false;
  if (fr == tr) {
    const int step = (tc > fc) ? 1 : -1;
    for (int c = fc + step; c != tc; c += step) {
      if (board[Idx(static_cast<uint8_t>(fr), static_cast<uint8_t>(c))] !=
          kEmpty) {
        return false;
      }
    }
  } else {
    const int step = (tr > fr) ? 1 : -1;
    for (int r = fr + step; r != tr; r += step) {
      if (board[Idx(static_cast<uint8_t>(r), static_cast<uint8_t>(fc))] !=
          kEmpty) {
        return false;
      }
    }
  }
  return true;
}

bool CannonAttacks(const XqB& board, uint8_t from, uint8_t target) noexcept {
  // Cannon attack semantics differ from move semantics: a cannon
  // *attacks* a target only if it can capture there (there is exactly
  // one piece — the screen — between source and target).
  const int fr = Row(from), fc = Col(from);
  const int tr = Row(target), tc = Col(target);
  if (fr != tr && fc != tc) return false;
  if (from == target) return false;
  int screens = 0;
  if (fr == tr) {
    const int step = (tc > fc) ? 1 : -1;
    for (int c = fc + step; c != tc; c += step) {
      if (board[Idx(static_cast<uint8_t>(fr), static_cast<uint8_t>(c))] !=
          kEmpty) {
        ++screens;
      }
    }
  } else {
    const int step = (tr > fr) ? 1 : -1;
    for (int r = fr + step; r != tr; r += step) {
      if (board[Idx(static_cast<uint8_t>(r), static_cast<uint8_t>(fc))] !=
          kEmpty) {
        ++screens;
      }
    }
  }
  return screens == 1;
}

bool SoldierAttacks(const XqB& /*board*/, uint8_t from, uint8_t target,
                    XqP player) noexcept {
  const int fr = Row(from), fc = Col(from);
  const int tr = Row(target), tc = Col(target);
  const int forward = player ? -1 : 1;
  // Forward by one.
  if (tr == fr + forward && tc == fc) return true;
  // Sideways once across the river.
  if (!OnOwnSide(fr, player)) {
    if (tr == fr && std::abs(tc - fc) == 1) return true;
  }
  return false;
}

}  // namespace

const std::array<std::array<uint64_t, kNumPieceCodes>, kBoardCells>
    kZobristPieceKeys = MakeZobristPieceKeys();

const uint64_t kZobristBlackToMove = MakeZobristBlackKey();

uint64_t HashBoard(const XqB& board, XqP current_player) noexcept {
  uint64_t h = 0;
  for (uint8_t i = 0; i < kBoardCells; ++i) {
    if (board[i] != kEmpty) {
      h ^= kZobristPieceKeys[i][ZobristPieceIndex(board[i])];
    }
  }
  if (current_player) {
    h ^= kZobristBlackToMove;
  }
  return h;
}

// ============================================================================
// Move generators.
// ============================================================================

void EmitGeneralMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, size_t& count) noexcept {
  const int r = Row(from), c = Col(from);
  static constexpr int kDR[] = {-1, 1, 0, 0};
  static constexpr int kDC[] = {0, 0, -1, 1};
  for (int i = 0; i < 4; ++i) {
    TryPushPalaceMove(board, from, r + kDR[i], c + kDC[i], player, out, count);
  }
}

void EmitAdvisorMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, size_t& count) noexcept {
  const int r = Row(from), c = Col(from);
  static constexpr int kDR[] = {-1, -1, 1, 1};
  static constexpr int kDC[] = {-1, 1, -1, 1};
  for (int i = 0; i < 4; ++i) {
    TryPushPalaceMove(board, from, r + kDR[i], c + kDC[i], player, out, count);
  }
}

void EmitElephantMoves(const XqB& board, uint8_t from, XqP player,
                       std::span<XqA> out, size_t& count) noexcept {
  const int r = Row(from), c = Col(from);
  static constexpr int kDR[] = {-2, -2, 2, 2};
  static constexpr int kDC[] = {-2, 2, -2, 2};
  for (int i = 0; i < 4; ++i) {
    const int tr = r + kDR[i];
    const int tc = c + kDC[i];
    if (!InBounds(tr, tc)) continue;
    if (!OnOwnSide(tr, player)) continue;  // Cannot cross river
    const int er = r + kDR[i] / 2;
    const int ec = c + kDC[i] / 2;
    if (board[Idx(static_cast<uint8_t>(er), static_cast<uint8_t>(ec))] !=
        kEmpty) {
      continue;  // Eye blocked
    }
    if (IsOwnedBy(
            board[Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))],
            player)) {
      continue;
    }
    out[count++] =
        XqA{from, Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))};
  }
}

void EmitHorseMoves(const XqB& board, uint8_t from, XqP player,
                    std::span<XqA> out, size_t& count) noexcept {
  const int r = Row(from), c = Col(from);
  // 8 L-targets and the orthogonal-leg cell that must be empty.
  struct Move {
    int dr, dc, leg_dr, leg_dc;
  };
  static constexpr Move kMoves[] = {
      {-2, -1, -1, 0}, {-2, 1, -1, 0}, {2, -1, 1, 0}, {2, 1, 1, 0},
      {-1, -2, 0, -1}, {1, -2, 0, -1}, {-1, 2, 0, 1}, {1, 2, 0, 1},
  };
  for (const Move& m : kMoves) {
    const int tr = r + m.dr;
    const int tc = c + m.dc;
    if (!InBounds(tr, tc)) continue;
    const int leg_r = r + m.leg_dr;
    const int leg_c = c + m.leg_dc;
    if (board[Idx(static_cast<uint8_t>(leg_r), static_cast<uint8_t>(leg_c))] !=
        kEmpty) {
      continue;
    }
    if (IsOwnedBy(
            board[Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))],
            player)) {
      continue;
    }
    out[count++] =
        XqA{from, Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))};
  }
}

void EmitChariotMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, size_t& count) noexcept {
  const int r = Row(from), c = Col(from);
  static constexpr int kDR[] = {-1, 1, 0, 0};
  static constexpr int kDC[] = {0, 0, -1, 1};
  for (int dir = 0; dir < 4; ++dir) {
    int tr = r + kDR[dir];
    int tc = c + kDC[dir];
    while (InBounds(tr, tc)) {
      const int8_t code =
          board[Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))];
      if (code == kEmpty) {
        out[count++] =
            XqA{from, Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))};
      } else {
        if (!IsOwnedBy(code, player)) {
          out[count++] = XqA{
              from, Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))};
        }
        break;
      }
      tr += kDR[dir];
      tc += kDC[dir];
    }
  }
}

void EmitCannonMoves(const XqB& board, uint8_t from, XqP player,
                     std::span<XqA> out, size_t& count) noexcept {
  const int r = Row(from), c = Col(from);
  static constexpr int kDR[] = {-1, 1, 0, 0};
  static constexpr int kDC[] = {0, 0, -1, 1};
  for (int dir = 0; dir < 4; ++dir) {
    // Phase 1: empty cells.
    int tr = r + kDR[dir];
    int tc = c + kDC[dir];
    while (InBounds(tr, tc)) {
      const int8_t code =
          board[Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))];
      if (code != kEmpty) break;
      out[count++] =
          XqA{from, Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))};
      tr += kDR[dir];
      tc += kDC[dir];
    }
    // Phase 2: skip exactly one screen, then look for an enemy capture.
    if (!InBounds(tr, tc)) continue;
    tr += kDR[dir];
    tc += kDC[dir];
    while (InBounds(tr, tc)) {
      const int8_t code =
          board[Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))];
      if (code != kEmpty) {
        if (!IsOwnedBy(code, player)) {
          out[count++] = XqA{
              from, Idx(static_cast<uint8_t>(tr), static_cast<uint8_t>(tc))};
        }
        break;
      }
      tr += kDR[dir];
      tc += kDC[dir];
    }
  }
}

void EmitSoldierMoves(const XqB& board, uint8_t from, XqP player,
                      std::span<XqA> out, size_t& count) noexcept {
  const int r = Row(from), c = Col(from);
  const int forward = player ? -1 : 1;
  // Forward.
  TryPushMove(board, from, r + forward, c, player, out, count);
  // Sideways after crossing the river (Red: row >= 5; Black: row <= 4).
  const bool crossed = player ? (r <= 4) : (r >= 5);
  if (crossed) {
    TryPushMove(board, from, r, c - 1, player, out, count);
    TryPushMove(board, from, r, c + 1, player, out, count);
  }
}

void EmitPseudoLegalMoves(const XqB& board, XqP player, std::span<XqA> out,
                          size_t& count) noexcept {
  count = 0;
  for (uint8_t cell = 0; cell < kBoardCells; ++cell) {
    const int8_t code = board[cell];
    if (!IsOwnedBy(code, player)) continue;
    switch (PieceType(code)) {
      case kRedGeneral:
        EmitGeneralMoves(board, cell, player, out, count);
        break;
      case kRedAdvisor:
        EmitAdvisorMoves(board, cell, player, out, count);
        break;
      case kRedElephant:
        EmitElephantMoves(board, cell, player, out, count);
        break;
      case kRedHorse:
        EmitHorseMoves(board, cell, player, out, count);
        break;
      case kRedChariot:
        EmitChariotMoves(board, cell, player, out, count);
        break;
      case kRedCannon:
        EmitCannonMoves(board, cell, player, out, count);
        break;
      case kRedSoldier:
        EmitSoldierMoves(board, cell, player, out, count);
        break;
      default:
        break;
    }
  }
}

// ============================================================================
// Position predicates.
// ============================================================================

uint8_t FindGeneral(const XqB& board, XqP player) noexcept {
  const int8_t target_code = player ? kBlackGeneral : kRedGeneral;
  for (uint8_t i = 0; i < kBoardCells; ++i) {
    if (board[i] == target_code) return i;
  }
  return kBoardCells;
}

bool IsCellAttacked(const XqB& board, uint8_t target,
                    XqP attacker_player) noexcept {
  for (uint8_t cell = 0; cell < kBoardCells; ++cell) {
    const int8_t code = board[cell];
    if (!IsOwnedBy(code, attacker_player)) continue;
    bool attacks = false;
    switch (PieceType(code)) {
      case kRedGeneral:
        attacks = GeneralAttacks(board, cell, target);
        break;
      case kRedAdvisor:
        attacks = AdvisorAttacks(board, cell, target);
        break;
      case kRedElephant:
        attacks = ElephantAttacks(board, cell, target, attacker_player);
        break;
      case kRedHorse:
        attacks = HorseAttacks(board, cell, target);
        break;
      case kRedChariot:
        attacks = ChariotAttacks(board, cell, target);
        break;
      case kRedCannon:
        attacks = CannonAttacks(board, cell, target);
        break;
      case kRedSoldier:
        attacks = SoldierAttacks(board, cell, target, attacker_player);
        break;
      default:
        break;
    }
    if (attacks) return true;
  }
  return false;
}

bool IsInCheck(const XqB& board, XqP player) noexcept {
  const uint8_t gen = FindGeneral(board, player);
  if (gen == kBoardCells) return true;  // Missing General is a loss.
  return IsCellAttacked(board, gen, !player);
}

bool IsFlyingGenerals(const XqB& board) noexcept {
  const uint8_t red_gen = FindGeneral(board, false);
  const uint8_t blk_gen = FindGeneral(board, true);
  if (red_gen == kBoardCells || blk_gen == kBoardCells) return false;
  if (Col(red_gen) != Col(blk_gen)) return false;
  const int col = Col(red_gen);
  const int top = Row(red_gen) < Row(blk_gen) ? Row(red_gen) : Row(blk_gen);
  const int bot = Row(red_gen) < Row(blk_gen) ? Row(blk_gen) : Row(red_gen);
  for (int r = top + 1; r < bot; ++r) {
    if (board[Idx(static_cast<uint8_t>(r), static_cast<uint8_t>(col))] !=
        kEmpty) {
      return false;
    }
  }
  return true;
}

}  // namespace az::game::xq::internal
