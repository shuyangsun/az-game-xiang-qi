#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_GAME_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_GAME_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

#include "alpha-zero-api/game.h"

namespace az::game::xq {

/**
 * @brief Board side dimensions and derived constants.
 *
 * Xiang Qi is played on a 9-file × 10-rank grid; pieces occupy
 * intersections, giving 90 cells. `XqB` stores one signed byte per
 * cell using row-major layout (`r * kBoardCols + c`). See
 * `memory/game_design_details/board_encoding.md` for the cell value
 * codes and starting layout.
 */
constexpr uint8_t kBoardRows = 10;
constexpr uint8_t kBoardCols = 9;
constexpr uint8_t kBoardCells = kBoardRows * kBoardCols;  // 90

/**
 * @brief Board representation: signed byte per cell.
 *
 * Encoding (see `memory/game_design_details/board_encoding.md`):
 *   0       — empty
 *  +1..+7   — Red pieces (General, Advisor, Elephant, Horse,
 *             Chariot, Cannon, Soldier)
 *  -1..-7   — Black pieces (same order, negated sign)
 */
using XqB = std::array<int8_t, kBoardCells>;

/**
 * @brief One Xiang Qi action: move a piece from `from` to `to`.
 *
 * Both fields are flat cell indices in `[0, kBoardCells)` (= 0..89).
 * `XqA{kBoardCells, kBoardCells}` is the reserved "no action"
 * sentinel and is never returned by `ValidActionsInto()`.
 */
struct XqA {
  uint8_t from;
  uint8_t to;
};

/**
 * @brief Player type. `Player1` (Red) moves first; `Player2` is Black.
 *
 * Uses the `BinaryPlayer` convention so the REPL prints "Player 1" /
 * "Player 2" lines automatically.
 */
using XqP = bool;
inline constexpr XqP kRed = false;
inline constexpr XqP kBlack = true;

/**
 * @brief Error type for `XqGame` failure modes.
 *
 * Only used off the MCTS hot path: `ActionFromString` and
 * `XqDeserializer::Deserialize` produce these errors.
 * `ApplyActionInPlace` does not validate its input — the engine is
 * responsible for only ever passing actions returned by
 * `ValidActionsInto()`.
 */
enum class XqError : uint8_t {
  kUnknownError = 0,
  kNotImplemented,
  kInvalidActionFormat,
  kInvalidActionFromCell,
  kInvalidActionToCell,
  kInvalidPolicyOutputSize,
};

template <typename T>
using XqResult = std::expected<T, XqError>;

using XqStatus = XqResult<void>;

/**
 * @brief A value-typed implementation of the Xiang Qi game.
 *
 * Conforms to `::az::game::api::Game`. The MCTS engine is templated
 * on this concrete type, so there is no virtual dispatch and the
 * compiler can see `sizeof(XqGame)`. Apply transitions in place via
 * `ApplyActionInPlace` and step back via `UndoLastAction`; both are
 * the allocation-free contract used on the MCTS hot path. For
 * cold-path snapshots, prefer the free function
 * `::az::game::api::ApplyAction(game, action)`, which is defined
 * once for any conforming `Game`.
 *
 * The board, action, and player encodings are documented in
 * `memory/game_design.md` and its detail files. Threefold-repetition
 * detection lives inside the game state via Zobrist hashing — see
 * `memory/game_design_details/repetition.md`.
 */
class XqGame {
 public:
  // ----------------------------- Associated types
  // -----------------------------
  using board_t = XqB;
  using action_t = XqA;
  using player_t = XqP;
  using error_t = XqError;

  // ------------------------------ Static contract
  // -----------------------------

  /**
   * @brief Number of past states the serializer needs as input.
   *
   * Xiang Qi is treated as Markov from the network's point of view:
   * threefold repetition is tracked inside `XqGame` via a Zobrist
   * hash log, so the serializer never needs past states. Keeping
   * this at 0 also avoids inflating the engine's per-node history
   * `RingBuffer<XqGame, std::array<XqGame, N>>`.
   */
  static constexpr size_t kHistoryLookback = 0;

  /**
   * @brief Cardinality of the full action space — fixed-size policy
   * head.
   *
   * Each action is a `(from, to)` pair of cell indices. The mapping
   * `PolicyIndex(a) = a.from * kBoardCells + a.to` is bijective on
   * `[0, kBoardCells^2) = [0, 8100)`. Most slots correspond to
   * physically impossible moves; they are masked out by
   * `ValidActionsInto()` and stay zero in the policy serializer.
   */
  static constexpr size_t kPolicySize =
      static_cast<size_t>(kBoardCells) * kBoardCells;

  /**
   * @brief Per-state legal-action ceiling for compact policy heads.
   *
   * The dense policy space remains `kPolicySize == 8100`, but any
   * reachable Xiang Qi state has at most 104 legal actions. Compact
   * serializers size their fixed action rows against this tighter
   * ceiling while dense serializers still scatter/gather over
   * `kPolicySize`.
   */
  static constexpr size_t kMaxLegalActions = 104;

  /**
   * @brief Self-play hard cap on `CurrentRound()`.
   *
   * 300 plies (150 moves per side) — generous headroom over typical
   * games (~80–120 plies) while bounding the size of the in-state
   * action / position history arrays. Once `CurrentRound() >=
   * *kMaxRounds`, `IsOver()` returns `true` and `GetScore` is `0`
   * for both players (technical draw).
   */
  static constexpr std::optional<uint32_t> kMaxRounds =
      static_cast<uint32_t>(300);

  // ------------------------------- Constructors
  // -------------------------------

  /**
   * @brief Construct the standard starting position with Red to move.
   */
  XqGame() noexcept;

  /**
   * @brief Construct the standard starting position with the given
   * starting player.
   */
  explicit XqGame(XqP starting_player) noexcept;

  /**
   * @brief Construct from an arbitrary snapshot.
   *
   * Used by augmenters to materialize transformed positions. The
   * resulting game's `LastAction()` / `LastPlayer()` observers are
   * seeded from `last_action`, but undo metadata for that historical
   * action is unavailable, so `UndoLastAction()` will not reverse it.
   * Actions applied after construction remain fully undoable. The
   * position-history log is seeded with the current position only;
   * older history is left empty, so threefold-repetition detection on
   * snapshot-constructed games is not meaningful.
   */
  XqGame(const XqB& board, XqP current_player, uint32_t current_round,
         std::optional<XqA> last_action) noexcept;

  XqGame(const XqGame& other) noexcept = default;
  XqGame(XqGame&& other) noexcept = default;
  XqGame& operator=(const XqGame& other) noexcept = default;
  XqGame& operator=(XqGame&& other) noexcept = default;
  ~XqGame() = default;

  // -------------------------------- Observers
  // ---------------------------------

  /**
   * @brief Get the current game board state.
   */
  [[nodiscard]] const XqB& GetBoard() const noexcept;

  /**
   * @brief Get the current round number (number of half-moves
   * played so far).
   */
  [[nodiscard]] uint32_t CurrentRound() const noexcept;

  /**
   * @brief Get the current player. Red (`false`) moves first.
   */
  [[nodiscard]] XqP CurrentPlayer() const noexcept;

  /**
   * @brief Get the player from last round, or `std::nullopt` if no
   * action has been played yet.
   */
  [[nodiscard]] std::optional<XqP> LastPlayer() const noexcept;

  /**
   * @brief Get the last action taken, or `std::nullopt` if no
   * action has been played yet.
   */
  [[nodiscard]] std::optional<XqA> LastAction() const noexcept;

  /**
   * @brief Canonical board representation from the current player's
   * perspective.
   *
   * Returns a copy of the board where the current player's pieces
   * are positive and the opponent's are negative AND the board is
   * rotated 180 degrees so own pieces always occupy rows `0..4`
   * from the same forward-facing perspective. If Red is to move, the
   * board is returned unchanged (Red is already + and on top); if
   * Black is to move, every non-zero cell is negated and the board is
   * rotated by mapping `(r, c)` to
   * `(kBoardRows - 1 - r, kBoardCols - 1 - c)`.
   *
   * Pairs with `CanonicalAction` so policy slots written by
   * `XqSerializer` live in the same canonical frame as the board
   * input.
   */
  [[nodiscard]] XqB CanonicalBoard() const noexcept;

  /**
   * @brief Canonical-frame remapping of an `(from, to)` action.
   *
   * Returns the action expressed in the same coordinate frame as
   * `CanonicalBoard()`. For Red, this is the identity. For Black,
   * each cell is rotated 180 degrees (`r → kBoardRows - 1 - r`,
   * `c → kBoardCols - 1 - c`), matching the board transform applied
   * by `CanonicalBoard()`. The mapping is self-inverse — calling it
   * twice returns the original action.
   *
   * `XqSerializer` and `XqDeserializer` both index policy slots via
   * `PolicyIndex(CanonicalAction(a))` so the model sees consistent
   * canonical-frame input and output regardless of side to move.
   */
  [[nodiscard]] XqA CanonicalAction(const XqA& action) const noexcept;

  /**
   * @brief Write all legal moves for the current player into
   * caller-owned storage and return the count.
   *
   * Writes the legal actions into `out[0 .. count)` and returns
   * `count`. Entries at indices `>= count` are unspecified and must
   * be ignored by the caller. The buffer is caller-owned — sized to
   * `kMaxLegalActions` — and the call is allocation-free, matching
   * the v0.2.1 `::az::game::api::Game` contract.
   *
   * Ordering is deterministic for a given game state: same indices
   * receive the same actions on every call. Iteration is cell-major
   * (cells `0..89`) and within each cell follows the
   * piece-type-specific direction order documented in
   * `memory/game_design_details/action_encoding.md`. No duplicates.
   *
   * Returns `0` if and only if `IsOver()` returns `true`. Xiang Qi
   * has no "pass" — a side with no legal moves loses (checkmate or
   * stalemate, both scored as a loss for the side to move).
   */
  [[nodiscard]] size_t ValidActionsInto(
      std::array<XqA, kMaxLegalActions>& out) const noexcept;

  /**
   * @brief Number of times the current position has occurred so far,
   * clamped to `[1, 3]`.
   *
   * Returns `1` for a position seen for the first time (including
   * the initial state), `2` for the second occurrence, and `3` once
   * the position has been seen three or more times — at which point
   * `IsOver()` reports a draw by threefold repetition. The bound of
   * `3` matches the rule: the network never needs to distinguish
   * higher counts because the game terminates there.
   *
   * Counts are based on the Zobrist position-history log; snapshot-
   * constructed games (no playthrough history) always report `1`.
   */
  [[nodiscard]] uint8_t CurrentPositionRepeatCount() const noexcept;

  /**
   * @brief Whether the game has reached a terminal state.
   *
   * Returns `true` if any of the following holds:
   *   1. The side to move is in check and has no legal move
   *      (checkmate).
   *   2. The side to move has no legal move and is not in check
   *      (stalemate; counted as a loss for the side to move per
   *      Asian rules).
   *   3. The current position has occurred three times across the
   *      game (threefold repetition; draw).
   *   4. `CurrentRound() >= *kMaxRounds` (engine ply cap; draw).
   */
  [[nodiscard]] bool IsOver() const noexcept;

  /**
   * @brief Score for `player` in the current state.
   *
   * Returns `+1.0` for the winner, `-1.0` for the loser, `0.0` for
   * a draw or while the game is not yet over. Only guaranteed to
   * be meaningful once `IsOver()` is `true`.
   */
  [[nodiscard]] float GetScore(const XqP& player) const noexcept;

  // ------------------------------ Policy layout
  // -------------------------------

  /**
   * @brief Map an `XqA` to its slot in the fixed-size policy head.
   *
   * `PolicyIndex(a) = a.from * kBoardCells + a.to`. Bijective over
   * `XqA{from, to}` with `from`, `to` in `[0, kBoardCells)`. Slots
   * for physically impossible moves (e.g., cell pairs unreachable by
   * any piece) are still valid policy slots; they are masked by
   * `ValidActions()` and stay zero in the policy serializer.
   */
  [[nodiscard]] size_t PolicyIndex(const XqA& action) const noexcept;

  // -------------------------------- Mutation
  // ----------------------------------

  /**
   * @brief Apply `action` to this state in place.
   *
   * Moves the piece at `action.from` to `action.to`, capturing any
   * enemy piece on the destination, flips the side to move,
   * increments the round, and updates the Zobrist position-history
   * log. Allocation-free.
   *
   * The caller is responsible for passing only actions returned by
   * `ValidActions()`; behavior for an invalid action is undefined
   * (the move will execute regardless of legality, but the
   * resulting state may violate Xiang Qi rules).
   */
  void ApplyActionInPlace(const XqA& action) noexcept;

  /**
   * @brief Reverse the most recent `ApplyActionInPlace`.
   *
   * Restores the piece on `from`, restores any captured piece on
   * `to`, decrements the round, flips the side to move, and pops
   * the Zobrist position-history entry. Allocation-free. No-op if
   * there is nothing to undo. Supports rollback to depth
   * `*kMaxRounds`.
   */
  void UndoLastAction() noexcept;

  // --------------------------- String Conversions ----------------------------

  /**
   * @brief Human-readable string of the current board state.
   *
   * Formats the 9×10 board with row labels (0..9) and column labels
   * (a..i). Pieces are printed as one-character glyphs (uppercase
   * for Red, lowercase for Black). Used in the REPL and during
   * debugging.
   */
  [[nodiscard]] std::string BoardReadableString() const noexcept;

  /**
   * @brief Parse a human-readable action string of the form
   * `<from_col><from_row>-<to_col><to_row>`, e.g., `"b0-c2"`.
   *
   * `<col>` is `'a'..'i'` (case-insensitive), `<row>` is `'0'..'9'`.
   * Returns the parsed `XqA` or an `XqError` describing why parsing
   * failed.
   */
  [[nodiscard]] XqResult<XqA> ActionFromString(
      std::string_view action_str) const noexcept;

  /**
   * @brief Format an action as `<from_col><from_row>-<to_col><to_row>`
   * (lower-case columns), e.g., `"b0-c2"`. Deterministic; produces
   * a distinct string for each distinct action and round-trips
   * through `ActionFromString`.
   */
  [[nodiscard]] std::string ActionToString(const XqA& action) const noexcept;

 private:
  // Fixed-size action history. Sized to `*kMaxRounds` so the engine
  // can undo the deepest possible rollout without reallocating.
  static constexpr size_t kHistoryCap = static_cast<size_t>(kMaxRounds.value());

  // Board state. Default-initialized to the standard starting
  // position by the no-arg constructor.
  XqB board_{};

  // Half-move counter. `round_ == 0` is a freshly-constructed game.
  uint32_t round_ = 0;

  // Side to move. Red (`kRed`) moves first.
  XqP current_player_ = kRed;

  // Stack of applied actions. Slot `i` holds the action that brought
  // the game to round `i + 1`, or the no-action sentinel when no
  // reversible action is known for that ply.
  std::array<XqA, kHistoryCap> action_history_{};

  // Per-round Zobrist hash log. `position_history_[i]` is the hash
  // of the position at the start of round `i`. The array has one
  // extra entry so the terminal max-round position can be logged.
  std::array<uint64_t, kHistoryCap + 1> position_history_{};

  // Whether the corresponding position-history slot contains a real
  // hash. Snapshot-constructed games may only know the current slot.
  std::array<uint8_t, kHistoryCap + 1> position_history_valid_{};

  // Per-ply undo metadata: low nibble = captured piece magnitude
  // (0 if no capture), high bit = sign of captured piece (1 if
  // Black), or the unavailable sentinel for snapshot-seeded actions.
  // Allows `UndoLastAction` to restore the captured piece without
  // consulting any external table when metadata is available.
  std::array<uint8_t, kHistoryCap> apply_undo_log_{};

  // Running Zobrist hash of the current position. Updated
  // incrementally on Apply/Undo.
  uint64_t position_hash_ = 0;

  [[nodiscard]] bool IsThreefoldRepetition() const noexcept;
};

static_assert(::az::game::api::Game<XqGame>,
              "XqGame must satisfy ::az::game::api::Game.");

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_GAME_H_
