# Xiang Qi Unittest Checklists

TODO(TASK-REVIEW-GAME-FR-COV): review functional requirements test coverage.

TODO(TASK-UNITTEST-CHECKLIST): use `updating-unittest-checklists` skill.

Add game-rule-derived items from `game_rules.md` (the
`updating-unittest-checklists` skill walks through how). The bullets
below are API-contract FRs that apply to every game, regardless of
rules — keep them here as the floor, then add game-specific FRs above
each section.

## Game contract (from [api_contract.md](./api_contract.md))

- `static_assert(::az::game::api::Game<XqGame>)`
  compiles. (Implicit — failure is a build error, but a dedicated FR
  documents the requirement.)
- `BoardReadableString` returns a non-empty string and is distinct
  across representative boards (initial, mid-game, terminal).
- `ActionToString` is deterministic and produces a distinct string per
  distinct action in `ValidActions()`.
- `ActionFromString(ActionToString(a))` round-trips for every action
  `a` reachable through legal play.
- `ActionFromString` returns an `error_t` (not the unknown default)
  for representative malformed inputs (empty, garbage, partial).
- `PolicyIndex` is a bijection from `Action` into `[0, kPolicySize)`.
- `ValidActions()` is deterministic for a given game state — calling
  it twice in a row returns the same vector.
- `ValidActions()` is empty if and only if `IsOver()` returns `true`.
- `LastAction()` and `LastPlayer()` are `std::nullopt` on a
  freshly-constructed game; both are populated after the first
  `ApplyActionInPlace` call.
- `IsOver()` returns `true` once `CurrentRound() >= *kMaxRounds`
  (when `kMaxRounds` is set).
- `GetScore(player)` is in `[-1, +1]` for every reachable state and
  for every `player`. Conventional values: `+1` win, `-1` loss,
  `0` draw / ongoing.

## Mutation contract (from [mcts_constraints.md](./mcts_constraints.md))

- `ApplyActionInPlace` followed by `UndoLastAction` returns the game
  to its previous observable state — board, current player, round,
  `LastAction`, `LastPlayer`. Verify across the deepest MCTS rollout
  the engine will run, not just one apply / undo.
- Apply N actions then Undo N times returns to the original game
  state (full rollback).
- Copy- and move-construction preserve every observable field.

## Serializer / deserializer contract

- `SerializeCurrentState` and `SerializePolicyOutput` return
  fixed-length vectors across reachable game states.
- `SerializeCurrentState` accepts a partial `RingBufferView` (size <
  `kHistoryLookback`) without crashing, and zero-pads or otherwise
  handles missing history channels deterministically.
- `SerializePolicyOutput` slot 0 carries `target.z` (default layout)
  and policy slots not in `ValidActions()` are zero.
- `Deserialize(SerializePolicyOutput(target))` recovers an
  `Evaluation` whose `value` and `probabilities` match `target.z` and
  `target.pi` within numerical tolerance.
- `Deserialize` returns the documented `error_t` when the input
  vector has the wrong length.

## Augmenter contract (from [augmentation_strategy.md](./augmentation_strategy.md))

- Inference-time `Augment` returns one entry per
  `XqAugmentation` member; each variant
  satisfies the same `Game` contract as the input.
- `Augment(game)[0]` is the identity (`game` itself, by observable
  state).
- Each augmented variant has the same `ValidActions().size()` as the
  input (symmetry preserves legality).
- `Interpret(original, Augment(original), evaluations)` returns
  probabilities whose length matches `original.ValidActions().size()`
  and whose ordering matches `original.ValidActions()`.
- `InverseTransformAction(transform_action(a, sym), sym) == a` for
  every supported `sym` and every representative action `a`.
- Training-time `Augment` returns one `(game, target)` pair per
  augmentation, with `target.pi` permuted to stay aligned with the
  augmented game's `ValidActions()` and `target.z` preserved.

## REPL contract (from [main_binary.md](./main_binary.md))

The interactive REPL in `src/main.cc` exercises every method above
end-to-end. The `[debug]` output lines (state vector length, policy
vector length, round-trip status, augmentation variants) are the
runtime expression of the contract — when any FR above breaks, those
lines change visibly.
