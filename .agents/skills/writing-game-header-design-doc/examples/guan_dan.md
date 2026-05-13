# Guan Dan Game Header Design

## Contents

- Player: `uint8_t`
- Action: `std::array<uint8_t, 15>`
- Error: `uint8_t (enum)`
- Board: `custom struct`
- Static contract: `kHistoryLookback`, `kPolicySize`, `kMaxRounds`, `PolicyIndex`
- Constructors and private data members

## Player

```cpp
namespace az::game::gd {

// Guan Dan player type, values 0 through 3.
using GdP = uint8_t;

}  // namespace az::game::gd
```

## Action

```cpp
namespace az::game::gd {

// 4 bits are enough; no need for 8.
enum class ActionType : uint8_t {
  kPass = 0,
  kSingle,
  kPair,
  kTriple,
  kFullHouse,
  kStraight,
  kTube,
  kPlate,
  kStraightFlush,
  kBomb,
  kJokerBomb,
  kTribute,
  kReturnTribute,
};

// 4 bits are enough; no need for 8.
enum class KeyRank : uint8_t {
  kUnknown = 0,
  kAce,
  k2,
  k3,
  k4,
  k5,
  k6,
  k7,
  k8,
  k9,
  k10,
  kJ,
  kQ,
  kK,
  kSmallJoker,
  kBigJoker,
};

// First 4 bits are the underlying value of the action type; last 4 bits
// are the underlying value of the key rank.
using PackedTrick = uint8_t;

// Guan Dan action type.
//
// First 8 bits are PackedTrick; each of the remaining 108 bits represents
// the presence of a card. Rank-major (A, 2, ..., Big Joker), suits in
// clubs/diamonds/hearts/spades. The last 15 * 8 - (8 + 108) = 4 bits are
// 0-padded.
using GdA = std::array<uint8_t, 15>;

}  // namespace az::game::gd
```

## Error

```cpp
enum class GdError : uint8_t {
  kUnknownError = 0,
  kNotImplemented,
  kInvalidConstructorArguments,
};
```

## Board

```cpp
namespace az::game::gd {

// 3 * (54 * 2) = 324 bits total. 3 bits per card encode 5 states: no
// holder (0b100) or player 0..3 (0b000..0b011). Rank-major; suits in
// clubs/diamonds/hearts/spades. First 54 * 3 = 162 bits are deck 0,
// second 162 bits are deck 1. Last (41 * 8) - 324 = 4 bits are 0-padded.
using Hands = std::array<uint8_t, 41>;

struct GdBoard {
  Hands hands;

  // First 4 bits = team 0 (players 0 and 2)
  // Last 4 bits  = team 1 (players 1 and 3)
  // Persistent match levels: 0 -> 2, 1 -> 3, ..., 11 -> K, 12 -> A.
  // A team at level A wins only by passing A (one player finishes first
  // and the partner does not finish last).
  uint8_t team_levels = 0;

  PackedTrick active_trick;

  // First 4 bits = player who set active_trick
  // Last 4 bits  = number of players who have finished
  uint8_t trick_player_num_finished_packed;

  // Four 2-bit finish slots packed into one byte. Slot i stores the
  // player who finished in place i.
  uint8_t finish_order_packed = 0;
};

using GdB = GdBoard;

}  // namespace az::game::gd
```

## Static contract

```cpp
namespace az::game::gd {

// Markov: the network input only needs the current state.
inline constexpr size_t kGdHistoryLookback = 0;

// Cardinality of the full action space (ignoring legality).
// 13 ActionType * 16 KeyRank = 208 PackedTrick values; for the masked
// scatter to work we just need a bijection into a fixed range. Reuse
// PackedTrick as the slot id, padded to a power of two.
inline constexpr size_t kGdPolicySize = 256;

// Cap to prevent infinite loops when an early-iteration network plays
// itself. ~150 ply is well above any realistic Guan Dan match length.
inline constexpr std::optional<uint32_t> kGdMaxRounds = 256;

}  // namespace az::game::gd
```

## Class and private data members

```cpp
namespace az::game::gd {

template <typename T>
using GdResult = std::expected<T, GdError>;

using GdStatus = GdResult<void>;

class GdGame {
 public:
  using board_t = GdB;
  using action_t = GdA;
  using player_t = GdP;
  using error_t = GdError;

  static constexpr size_t kHistoryLookback = kGdHistoryLookback;
  static constexpr size_t kPolicySize = kGdPolicySize;
  static constexpr std::optional<uint32_t> kMaxRounds = kGdMaxRounds;

  // Public constructors (value semantics).
  GdGame() noexcept = default;
  GdGame(
      // If nullopt, randomly generate the last game's finish order.
      // nullopt is different from 0, which represents a fresh game.
      std::optional<uint8_t> last_game_finish_order_packed,
      // If nullopt, random current levels are generated for both teams.
      std::optional<uint8_t> current_team_levels,
      // If nullopt, a random hand is dealt.
      std::optional<Hands>&& hands) noexcept;

  // Observers, PolicyIndex, ApplyActionInPlace, UndoLastAction,
  // string conversions ... see ::az::game::api::Game for the full
  // contract.

 private:
  GdB board_;
  GdP cur_player_ = 0;
  uint32_t round_ = 0;

  // Action history sized to `kMaxRounds` so MCTS can undo a full rollout
  // without allocations on the hot path.
  std::array<GdA, *kMaxRounds> action_history_;
  uint16_t history_size_ = 0;
};

static_assert(::az::game::api::Game<GdGame>);

}  // namespace az::game::gd
```
