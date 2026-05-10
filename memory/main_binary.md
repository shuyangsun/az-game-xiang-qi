# Interactive Main Binary

`src/main.cc` is an interactive REPL for Xiang Qi. It is
the canonical way for a human to manually exercise the game implementation
end-to-end and to spot regressions across the
game / serializer / deserializer / augmenter boundaries.

Do not change the REPL semantics. Implement the game so the REPL becomes
more useful as more methods are filled in.

## What the REPL does each round

For the current game state it calls and prints, in order:

- `CurrentRound()`, `CurrentPlayer()`, `LastPlayer()`, `LastAction()`
- `BoardReadableString()`
- `IsOver()`; if true, `GetScore()` for both players (Player=bool) or for
  the current and last player otherwise, then exit.
- `ValidActions()` count is printed every round; the `actions` REPL
  command lists each action's `ActionToString` form.
- `[debug]` lines for serialization:
  - `SerializeCurrentState()` length (with the engine-owned
    `RingBufferView` history)
  - `SerializePolicyOutput()` length on a probe `TrainingTarget`
    (`z=0.5`, uniform `pi`)
  - `Deserialize()` round-trip vs the probe → `match`, `MISMATCH`,
    `failed (...)`, or `skipped (... empty vector)`
- `[debug]` lines for augmentation:
  - inference-time `Augment()` variant count, then per-variant action
    count
  - training-time `Augment()` example count

## Action input format

The user types the action as a string — exactly what `ActionToString`
returns for one of the currently valid actions. The REPL parses it with
`ActionFromString`, then matches the parsed action against the
`ValidActions()` list by comparing string forms (so the game's `Action`
type does not need `operator==`).

This means the implementation must guarantee:

- `ActionToString` is deterministic and returns a distinct string per
  distinct action.
- `ActionFromString` accepts every string `ActionToString` produces.

If either is still a placeholder, the REPL refuses every typed action.
That is the loudest signal that string conversions are next on the
critical path.

REPL commands besides actions: `actions` lists valid actions, `help`
shows usage, `quit` exits.

## Player printing

`PlayerString` in `main.cc` handles three cases via `if constexpr`:

- `bool` → `"Player 1"` / `"Player 2"` (the `BinaryPlayer` default).
- Other integral types → `"Player <n>"`.
- Anything else → uses `operator<<(std::ostream&, const Player&)`.

Custom Player classes must therefore provide a stream insertion
operator. Mention this in [game_design.md](./game_design.md) if the
design uses a non-default Player type.

## How the binary degrades when methods are placeholders

The REPL is built so that an LLM agent working through
[tasks.md](./tasks.md) can run it after most tasks and see something
useful:

- After `GAME-CONSTRUCTOR-IMPL`: REPL prints round 0 and the placeholder
  board string, lists 0 valid actions, and idles at the prompt.
- After `GAME-BASIC-IMPL` and `GAME-STATE-IMPL`: round number and game
  termination begin to be honored.
- After `GAME-ACTION-IMPL`: `ValidActions()`, `PolicyIndex()`,
  `ApplyActionInPlace()`, and `UndoLastAction()` produce real moves; the
  REPL becomes playable via the `actions` list but typed strings still
  fail until string conversion is done.
- After `GAME-STR-IMPL`: the REPL accepts typed action strings.
- After `SERIALIZER-IMPL` + `DESERIALIZER-IMPL`: `[debug] round-trip:`
  flips from `skipped`/`failed` to `match`.
- After `AUGMENTATION-IMPL`: inference variant count equals
  `|`XqAugmentation`|`; training example count matches.

## Running it

```text
cmake --preset debug && cmake --build --preset debug
./build/debug/xq
```

Walk a real game from start to game over to verify each piece of the
implementation works together. See `PLAY-MAIN-VERIFY` in
[tasks.md](./tasks.md).
