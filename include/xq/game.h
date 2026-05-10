#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_GAME_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_GAME_H_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "alpha-zero-api/game.h"

namespace az::game::xq {

// TODO(TASK-UPDATE-GAME-HEADER): design board type, action type, and player
// type.
//
// Type aliases below are only placeholders generated from the Cookiecutter
// template.
//
// Board type: what is intuitive and efficient in memory? Do we need to
// sacrifice runtime efficiency for memory efficiency?
//
// Action type: can we use a small integer type (e.g., uint8_t) to represent all
// possible actions? What's the smallest type we can use here?
//
// Player type: is it a two-player game? Is it a group game? For most two-player
// games, boolean is sufficient.
using XqB = uint64_t;
using XqA = int;
using XqP = bool;

/**
 * @brief Error type for XqGame failure.
 */
enum class XqError : uint8_t {
  kUnknownError = 0,
  kNotImplemented,
};

template <typename T>
using XqResult = std::expected<T, XqError>;

using XqStatus = XqResult<void>;

/**
 * @brief A value-typed implementation of the Xiang Qi game.
 *
 * Conforms to `::az::game::api::Game`. The MCTS engine is templated on this
 * concrete type, so there is no virtual dispatch and the compiler can see
 * `sizeof(XqGame)`. Apply transitions in place via
 * `ApplyActionInPlace` and step back via `UndoLastAction`; both are the
 * allocation-free contract used on the MCTS hot path. For cold-path
 * snapshots, prefer the free function
 * `::az::game::api::ApplyAction(game, action)`, which is defined once for
 * any conforming `Game`.
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
   * TODO(TASK-UPDATE-GAME-HEADER): set this to the lookback depth required
   * by your network (Markov games keep 0; Atari-style games typically use
   * 4–8).
   *
   * Markov games declare 0; the engine still owns the history `RingBuffer`
   * but the view passed to the serializer is always empty.
   */
  static constexpr std::size_t kHistoryLookback = 0;

  /**
   * @brief Cardinality of the full action space — fixed-size policy head.
   *
   * TODO(TASK-UPDATE-GAME-HEADER): set this to the cardinality of your
   * action space, ignoring legality. The bijection between an
   * `XqA` and a slot in `[0, kPolicySize)` is given
   * by `PolicyIndex`.
   */
  static constexpr std::size_t kPolicySize = 1;

  /**
   * @brief Self-play hard cap on `CurrentRound()`.
   *
   * TODO(TASK-UPDATE-GAME-HEADER): set this to the maximum number of
   * rounds your game can possibly run. Use `std::nullopt` for genuinely
   * unbounded games. The cap exists so pathological loops in
   * early-iteration networks still terminate; if set, `IsOver()` must
   * return `true` once `CurrentRound() >= *kMaxRounds`.
   */
  static constexpr std::optional<uint32_t> kMaxRounds = std::nullopt;

  // ------------------------------- Constructors
  // -------------------------------

  /**
   * @brief Construct a fresh Xiang Qi game state.
   *
   * TODO(TASK-UPDATE-GAME-HEADER): tailor constructors to your game.
   * Decide whether a default constructor is meaningful, whether the
   * starting player needs to be passed in, and whether more constructors
   * are needed.
   */
  XqGame() noexcept = default;

  explicit XqGame(const XqP& starting_player) noexcept;

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
   * @brief Get the current round number.
   */
  [[nodiscard]] uint32_t CurrentRound() const noexcept;

  /**
   * @brief Get the current player.
   */
  [[nodiscard]] XqP CurrentPlayer() const noexcept;

  /**
   * @brief Get the player from last round, or `std::nullopt` if the game
   * has not started yet.
   */
  [[nodiscard]] std::optional<XqP> LastPlayer() const noexcept;

  /**
   * @brief Get the last action taken, or `std::nullopt` if the game has
   * not started yet.
   */
  [[nodiscard]] std::optional<XqA> LastAction() const noexcept;

  /**
   * @brief Canonical board representation from the current player's
   * perspective.
   *
   * TODO(TASK-HEADER-DOCSTR): tailor this docstring to be
   * Xiang Qi specific; describe the canonical
   * representation in detail without exceeding a reasonable docstring
   * length.
   *
   * For example, for Tic Tac Toe, the canonical form can be a 2D array
   * where the current player's pieces are 1, the opponent's are -1, and
   * empty cells are 0. For incomplete-information games (e.g., card
   * games), the canonical form should only include information visible to
   * the current player.
   *
   * Variations from different perspectives belong in augmenters rather
   * than here. See
   * https://github.com/shuyangsun/alpha-zero-api/blob/main/src/include/alpha-zero-api/augmenter.h
   */
  [[nodiscard]] XqB CanonicalBoard() const noexcept;

  /**
   * @brief All valid actions for the current player in the current state.
   *
   * TODO(TASK-HEADER-DOCSTR): tailor this docstring to be
   * Xiang Qi specific.
   *
   * Must be deterministic in the game state — a training tuple
   * `(s, π, z)` written under one ordering and replayed against a network
   * trained under another is corrupt.
   *
   * No duplicates. Empty if and only if `IsOver()` returns true. While the
   * game is not over, even if there are no "real" choices for the current
   * player, return at least one action (e.g., a "pass") because
   * `ApplyActionInPlace` requires an action.
   */
  [[nodiscard]] std::vector<XqA> ValidActions() const noexcept;

  /**
   * @brief Whether the game has reached a terminal state.
   *
   * If `kMaxRounds` is set, must return `true` once
   * `CurrentRound() >= *kMaxRounds`. After `IsOver()` is true,
   * `GetScore` is the only method called on this state.
   */
  [[nodiscard]] bool IsOver() const noexcept;

  /**
   * @brief Score for `player` in the current state.
   *
   * Only guaranteed to be meaningful when `IsOver()` is true. Conventional
   * range is `[-1, +1]` from the given player's perspective.
   */
  [[nodiscard]] float GetScore(const XqP& player) const noexcept;

  // ------------------------------ Policy layout
  // -------------------------------

  /**
   * @brief Map an `XqA` to its slot in the
   * fixed-size policy head.
   *
   * TODO(TASK-HEADER-DOCSTR): describe how the bijection works for
   * Xiang Qi.
   *
   * The returned index must be in `[0, kPolicySize)` and the mapping must
   * be a bijection over the entire action space (legal or not). The
   * default policy serializer/deserializer use this to scatter and gather
   * masked policy values.
   */
  [[nodiscard]] std::size_t PolicyIndex(const XqA& action) const noexcept;

  // -------------------------------- Mutation
  // ----------------------------------

  /**
   * @brief Apply `action` to this state in place.
   *
   * TODO(TASK-HEADER-DOCSTR): tailor this docstring to be
   * Xiang Qi specific.
   *
   * Must be allocation-free — this is the MCTS hot-path primitive. The
   * caller is responsible for passing only actions returned by
   * `ValidActions()`; behavior for an invalid action is undefined.
   */
  void ApplyActionInPlace(const XqA& action) noexcept;

  /**
   * @brief Reverse the most recent `ApplyActionInPlace`.
   *
   * TODO(TASK-HEADER-DOCSTR): tailor this docstring to be
   * Xiang Qi specific. Make sure your private state
   * (e.g., `action_history_`) supports the depth of undo MCTS will need.
   *
   * No-op if there is nothing to undo. Must be allocation-free.
   */
  void UndoLastAction() noexcept;

  // --------------------------- String Conversions ----------------------------

  /**
   * @brief Human-readable string of the current board state.
   *
   * Used in the terminal UI and during debugging. Should be readable by
   * both human and LLM players.
   */
  [[nodiscard]] std::string BoardReadableString() const noexcept;

  /**
   * @brief Parse a human-readable action string.
   */
  [[nodiscard]] XqResult<XqA> ActionFromString(
      std::string_view action_str) const noexcept;

  /**
   * @brief Format an action as a human-readable string.
   */
  [[nodiscard]] std::string ActionToString(const XqA& action) const noexcept;

 private:
  // TODO(TASK-UPDATE-GAME-HEADER): design private members to keep track of
  // the game state.
  //
  // The fields below are placeholders. Likely changes:
  //   - `last_action_` / `last_player_` are single-slot; an MCTS rollout
  //     will undo many actions in a row, so you'll usually want a
  //     fixed-size or std::vector-backed action history sized to the
  //     deepest MCTS rollout.
  //   - Some games need richer history (e.g., card-played stack, repeated
  //     position counters). Decide whether to bake those into `board_` or
  //     track them separately.
  uint32_t round_ = 0;
  XqB board_ = XqB{};
  XqP cur_player_ = XqP{};
  std::optional<XqA> last_action_ = std::nullopt;
  std::optional<XqP> last_player_ = std::nullopt;
};

static_assert(::az::game::api::Game<XqGame>,
              "XqGame must satisfy ::az::game::api::Game.");

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_GAME_H_
