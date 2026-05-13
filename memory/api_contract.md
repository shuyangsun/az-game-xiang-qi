# AlphaZero API Contract

A condensed reference to the upstream
[shuyangsun/alpha-zero-api](https://github.com/shuyangsun/alpha-zero-api)
contract this project implements. Read this once before working on
[game_design.md](./game_design.md) or any `*-IMPL` task — almost every
implementation decision is shaped by something below.

The full headers live under `src/include/alpha-zero-api/` in the upstream
repo (and in `build/debug/_deps/alphazeroapi-src/...` once the project
has been built — see the `finding-alpha-zero-api` skill).

## The `Game` concept

`XqGame` must satisfy `::az::game::api::Game`.
Failure is a hard compile error from the `static_assert` at the bottom
of [game.h](../include/xq/game.h).

The concept is value-semantic and statically dispatched — no virtual
methods, no `unique_ptr` factory, no `IGame<...>` base class. The MCTS
engine is templated on `XqGame` and stores
instances by value.

### Required associated types

- `board_t` / `action_t` / `player_t` / `error_t`

### Required `static constexpr` members

- `kHistoryLookback : size_t` — past states the serializer needs.
  Markov games declare 0. See [history_lookback.md](./history_lookback.md).
- `kPolicySize : size_t` — cardinality of the full action space,
  ignoring legality. Equals the network's policy-head width.
- `kMaxRounds : std::optional<uint32_t>` — self-play hard cap. If set,
  `IsOver()` must return `true` once `CurrentRound() >= *kMaxRounds`.
  Use `std::nullopt` for genuinely unbounded games.

### Required observers

| Method                | Returns                                             |
| --------------------- | --------------------------------------------------- |
| `GetBoard()`          | `const board_t&`                                    |
| `CurrentRound()`      | `uint32_t`                                          |
| `CurrentPlayer()`     | `player_t`                                          |
| `LastPlayer()`        | `std::optional<player_t>` (nullopt before any move) |
| `LastAction()`        | `std::optional<action_t>` (nullopt before any move) |
| `CanonicalBoard()`    | `board_t` from the current player's perspective     |
| `ValidActions()`      | `std::vector<action_t>` — deterministic, no dupes   |
| `IsOver()`            | `bool`                                              |
| `GetScore(player)`    | `float` in `[-1, +1]` from `player`'s perspective   |
| `PolicyIndex(action)` | `size_t` in `[0, kPolicySize)`; bijection           |

### Required mutation primitives

- `ApplyActionInPlace(const action_t&) -> void` — primary transition,
  must be allocation-free (it sits on the MCTS hot path).
- `UndoLastAction() -> void` — reverse the most recent apply, also
  allocation-free.

`G ApplyAction(const G&, const action_t&)` is a **free function** in the
API library. Concrete games never implement it themselves.

### Required string I/O

- `BoardReadableString() -> std::string` — printed by the REPL.
- `ActionToString(action) -> std::string` — deterministic, distinct per
  action. Round-trips with `ActionFromString`.
- `ActionFromString(string_view) -> std::expected<action_t, error_t>`.

## Output types

- `Evaluation { float value; std::vector<float> probabilities; }` — what
  the network produced. `value ∈ [-1, +1]` from the current player's
  perspective. `probabilities[i]` is the prior for
  `game.ValidActions()[i]`.
- `TrainingTarget { float z; std::vector<float> pi; }` — what the
  network is asked to learn. `z` is the actual game outcome from
  `GetScore(state.CurrentPlayer())`. `pi[i]` is the MCTS visit-count
  prior for `game.ValidActions()[i]`.

Deserializers produce `Evaluation`; policy serializers consume
`TrainingTarget`. Same ordering convention on both.

## Engine-owned history

The state serializer signature is

```cpp
SerializeCurrentState(const G& game, RingBufferView<G> history)
```

`history.Size() <= G::kHistoryLookback`. Index 0 is the most recent
past state preceding `game`; `game` itself is **not** in the view. The
engine owns the `RingBuffer<G>`. Markov games (`kHistoryLookback == 0`)
always see an empty view. See [history_lookback.md](./history_lookback.md).

## Augmenter contract

When `augmenter = yes`:

- `IInferenceAugmenter<G>::Augment(game) -> std::vector<G>` — return
  every equivalent position. **Convention**: `result[0]` is the
  identity. The order matches the
  `XqAugmentation` enum in
  [augmentation.h](../include/xq/augmentation.h).
- `IInferenceAugmenter<G>::Interpret(original, augmented, evals) ->
Evaluation` — combine per-variant evaluations back into one for
  `original`, inverting whatever symmetry was applied so the returned
  probabilities align with `original.ValidActions()`.
- `ITrainingAugmenter<G>::Augment(game, target) ->
std::vector<std::pair<G, TrainingTarget>>` — return every augmented
  `(game, target)` pair. The augmented `pi[i]` corresponds to the
  augmented game's `ValidActions()[i]`. `target.z` is preserved.

See [augmentation_strategy.md](./augmentation_strategy.md) for design
guidance.

## Defaults shipped by the API

`alpha-zero-api/defaults/` provides reusable building blocks that work
out of the box for many games. See [defaults.md](./defaults.md) for
when to use them vs. roll your own.
