# Xiang Qi Game Header Design

Design for [include/xq/game.h](../include/xq/game.h). Targets the
`::az::game::api::Game` concept; value-semantic, allocation-free
on `ApplyActionInPlace` / `UndoLastAction`.

## Contents

- [Type aliases](#type-aliases)
- [Static contract](#static-contract)
- [Constructors](#constructors)
- [Private state](#private-state)
- [Mutation primitives](#mutation-primitives)
- [Error type](#error-type)
- [Serializer / deserializer choice](#serializer--deserializer-choice)
- [Augmentation symmetry set](#augmentation-symmetry-set)
- Details
  - [Board encoding](./game_design_details/board_encoding.md)
  - [Action encoding](./game_design_details/action_encoding.md)
  - [Repetition tracking](./game_design_details/repetition.md)

## Type aliases

```cpp
using XqB = std::array<int8_t, 90>;  // row-major, r*9+c
struct XqA { uint8_t from; uint8_t to; };  // both in [0, 89]
using XqP = bool;                    // false=Red (P1), true=Black (P2)
enum class XqError : uint8_t { /* see below */ };
```

- **Player**: `bool`, with the `BinaryPlayer` convention. Red is
  `Player1` (`false`); Red moves first.
- **Action**: 2 bytes total. `from` and `to` are flat cell
  indices `r*9+c` for `r in [0,9]` and `c in [0,8]`.
  Sentinel `from == to == 90` is reserved as "no action" and is
  never returned by `ValidActions()`.
- **Board**: `std::array<int8_t, 90>`, 90 bytes. See
  [board_encoding.md](./game_design_details/board_encoding.md)
  for the cell value codes.

## Static contract

```cpp
static constexpr size_t   kHistoryLookback = 0;
static constexpr size_t   kPolicySize      = 90 * 90;  // 8100
static constexpr std::optional<uint32_t> kMaxRounds = 300;
```

- **`kHistoryLookback = 0`**. Game is Markov from the network's
  point of view: repetition counters live inside `XqGame`, so
  the serializer never needs past states. Avoids inflating
  per-node `RingBuffer<XqGame, std::array<XqGame, N>>` memory.
- **`kPolicySize = 8100`**. Action space cardinality is
  `from_cell * 90 + to_cell`. The mapping is dense (most slots
  are illegal) but trivial to compute and bijective.
- **`kMaxRounds = 300`**. Hard self-play cap (150 per side).
  Average human Xiang Qi games run ~80–120 plies; 300 leaves
  generous headroom while bounding `action_history_` and
  `position_history_` storage.

```cpp
[[nodiscard]] size_t PolicyIndex(const XqA& a) const noexcept {
  return static_cast<size_t>(a.from) * 90 + a.to;
}
```

## Constructors

```cpp
XqGame() noexcept;                                  // standard start
explicit XqGame(XqP starting_player) noexcept;      // pick side
// snapshot ctor used by augmenters; takes board, player, round,
// optional last action. NOT for MCTS use.
XqGame(XqB board, XqP current_player, uint32_t round,
       std::optional<XqA> last_action) noexcept;
```

Snapshot constructor mirrors the tic-tac-toe upstream pattern;
augmenters need it to materialize transformed positions without
replaying moves.

## Private state

```cpp
XqB board_{};                                  // 90 B
uint32_t round_ = 0;                           // 4 B
XqP current_player_ = false;                   // 1 B (Red starts)
std::array<XqA, *kMaxRounds> action_history_{}; // 600 B
std::array<uint64_t, *kMaxRounds> position_history_{};  // 2400 B
// Each Apply also stores: captured piece code + repetition-counter
// delta, packed into one byte per ply, used by Undo.
std::array<uint8_t, *kMaxRounds> apply_undo_log_{};     // 300 B
```

Total per instance: ~3.4 KiB. Significant but predictable; see
the memory note in
[repetition.md](./game_design_details/repetition.md).

## Mutation primitives

- `ApplyActionInPlace`:
  1. Read captured piece code from `board_[to]`.
  2. Move piece, clear source, flip `current_player_`, increment
     `round_`.
  3. Update Zobrist hash incrementally; push to
     `position_history_`.
  4. Pack captured piece + irreversible-flag into
     `apply_undo_log_[round_-1]`.
- `UndoLastAction`:
  1. No-op if `round_ == 0`.
  2. Decrement `round_`, flip `current_player_`.
  3. Reverse move using `action_history_` and
     `apply_undo_log_`.
  4. Pop `position_history_`.

Both are allocation-free (only fixed-size arrays touched).

## Error type

```cpp
enum class XqError : uint8_t {
  kUnknownError = 0,
  kNotImplemented,
  kInvalidActionFormat,
  kInvalidActionFromCell,
  kInvalidActionToCell,
  kInvalidPolicyOutputSize,
};
```

`error_t` is used by `ActionFromString` and `XqDeserializer`.
Not on the MCTS hot path — `ApplyActionInPlace` does not
validate; the engine guarantees only valid actions.

## Serializer / deserializer choice

- **Policy-output serializer**: use the canonical layout
  `[z, p_0, ..., p_{8099}]` (length 8101). Reuse
  `DefaultPolicyOutputSerializer<XqGame>` semantics inside
  `XqSerializer::SerializePolicyOutput`.
- **Policy-output deserializer**: implement directly because
  `XqDeserializer` returns `XqError`, not `std::string`. Layout
  matches the serializer; size validation rejects vectors whose
  length is not `kPolicySize + 1`.
- **State serializer**: custom — see the input encoding section
  in [board_encoding.md](./game_design_details/board_encoding.md).
  14 piece-type planes + 1 side-to-move plane = 15 planes of
  `9 x 10`. Output length 1350 floats.

## Augmentation symmetry set

The board has only **left/right mirror** symmetry — the river
divides the two sides asymmetrically and the palaces / pieces
are tied to specific files. Rotations break the river /
palaces; vertical flip would swap players (a perspective change,
not a symmetry).

```cpp
enum class XqAugmentation : uint8_t {
  kOriginal       = 0,
  kMirrorHorizontal = 1,
};
```

- `AugmentAll(game).size() == 2` always.
- `result[0]` is the identity, `result[1]` is the column-mirror.
- `InverseTransformAction(action, kMirrorHorizontal)` swaps each
  cell's column: `c -> 8 - c` for both `from` and `to`.
- Inference `Interpret` averages over the 2 variants.
