# MCTS Hot-Path Constraints

Performance and correctness constraints the Xiang Qi
implementation has to respect because the engine drives it from inside
the MCTS rollout loop. Read before any of the `GAME-*-IMPL` tasks.

## Value semantics, no virtual dispatch

`XqGame` is a plain value type. The engine is
templated on it directly, so:

- The compiler sees `sizeof(XqGame)`. Keep it
  small — the engine stores instances by value in the search tree, and
  in `std::array<G, N>` history buffers.
- Copy / move construction must work cleanly. The default
  `= default` versions are fine for any game whose private state is
  trivially copyable; non-trivial members (e.g.
  `std::vector<action_t> action_history_`) inherit the cost of
  copying themselves on every snapshot.
- No virtual methods. No `std::unique_ptr<IXqGame>`
  factory. The `static_assert(::az::game::api::Game<...>)` at the
  bottom of [game.h](../include/xq/game.h)
  is the gate.

## `ApplyActionInPlace` and `UndoLastAction` are allocation-free

These two methods sit on the inner MCTS rollout loop and are called
once per expanded edge. A single allocation per call multiplies into
hundreds of millions of allocations across a training cycle.

Concrete implications:

- Do not `push_back` into an unbounded `std::vector` inside `Apply`.
  Pre-size action history with a `std::array<action_t, MAX_DEPTH>` (or
  reserve once at construction).
- Do not allocate scratch space — keep computation in stack-local
  fixed-size buffers.
- `noexcept` is the right marker for both methods.

The free function `::az::game::api::ApplyAction(game, action)` does
copy + apply for cold-path snapshots. It is **not** the MCTS hot
path — do not implement a member `ApplyAction`; it would shadow the
free function.

## Undo depth must cover the deepest rollout

A single MCTS rollout can apply many actions in sequence and then undo
them in reverse to return to the root. The action history private to
`XqGame` must be sized for the deepest rollout
the engine will run.

The placeholder game template has

```cpp
std::optional<XqA> last_action_ = std::nullopt;
std::optional<XqP> last_player_ = std::nullopt;
```

which only supports one level of undo. Replace with a fixed-size
history sized to the deepest rollout (typically the same upper bound
as `kMaxRounds`, since a rollout cannot exceed game length):

```cpp
std::array<XqA, XqGame::kMaxRounds.value_or(MAX_DEPTH)>
    action_history_{};
uint16_t history_size_ = 0;
```

The tic-tac-toe reference implementation upstream uses exactly this
pattern.

## Determinism inside the engine

`ValidActions()` ordering is part of the contract. A training tuple
`(state, π, z)` is recorded under one ordering and replayed against a
network trained under another — if the orderings disagree, the policy
labels are scrambled. The ordering must depend only on the current
game state, never on time, RNG, or call history.

`PolicyIndex(action)` must be a **bijection** over the entire action
space. The default policy serializer scatters into the slot
`PolicyIndex(a)`; the deserializer gathers from it. A non-injective
mapping silently corrupts training.

## Error type

`error_t` is for `ActionFromString` and the deserializer. It is **not**
called on the MCTS hot path — `ApplyActionInPlace` does not validate
its input. The engine is responsible for only ever passing actions
returned by `ValidActions()`. Behavior on an invalid action is
undefined.

If the design needs validated apply for debug builds, add a separate
`std::expected<void, error_t> TryApply(...)` or wrap the call in
asserts that compile out under `-DNDEBUG`.
