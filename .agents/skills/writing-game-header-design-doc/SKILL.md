---
name: writing-game-header-design-doc
description: Write a design doc outlining the key elements of the game header file.
---

# Writing a game header design document

Design board, action, player, and error types for
[include/xq/game.h](../../../include/xq/game.h)
using your best judgment, plus the static contract the `Game` concept
requires (`kHistoryLookback`, `kPolicySize`, `kMaxRounds`, `PolicyIndex`).
Do not implement anything; only write the design doc in
[memory/game_design.md](../../../memory/game_design.md).

Ground the design doc in the rules document
[memory/game_rules.md](../../../memory/game_rules.md). Do not make
assumptions that are not clearly stated there. If the rules document is
ambiguous, ask the user to clarify, then update the rules document with
that clarification before proceeding. Although the rule book was written
for humans without much technical background, the design doc should be
written as a technical document for LLMs and software engineers. Include
code snippets when helpful.

## API header template

The Xiang Qi-specific C++ header is
[include/xq/game.h](../../../include/xq/game.h),
which declares a class that must satisfy the `::az::game::api::Game`
concept. The header may have pre-existing type definitions, but they are
likely placeholders if there are TODO comments around them. Use the
`finding-alpha-zero-api` skill if the game-specific API does not provide
enough information.

The `Game` concept is value-semantic and allocation-free on the hot path.
Concrete games are plain value types — no virtual interface, no
`unique_ptr` factory. The MCTS engine is templated on the concrete game
type, calls `ApplyActionInPlace` to advance state, and calls
`UndoLastAction` to step back during a rollout.

## Key design principles

Optimize for memory efficiency, since instances of this type will be
copied per node in the MCTS search tree. Do not worry about
serialization or deserialization yet; that will be handled by separate
serializer / deserializer / augmenter implementations. Focus on getting
the game logic and rules correct while keeping the memory footprint
minimal.

## Player

Usually a boolean for a two-player game, or a `uint8_t` for a game with
more than two players. For some games, a more complex packed
representation makes sense. For example, for werewolf, use the first 7
bits for the player ID and the last bit for whether the player is a
werewolf.

## Action

A single action that a player can take given the current state. Many
games have a "pass" action, which should be explicitly represented by
this type — an empty `ValidActions()` is reserved for "the game is
over." Optimize this type aggressively for memory footprint, especially
for games with a very large action space whose space complexity may grow
exponentially. Do not pack multiple actions into one instance: each
action must produce a distinct game state in the MCTS search tree.

## Error

Define an error type to represent invalid actions or game states.
Invalid game states can arise when constructor arguments are bad, or
when there is a bug in the game logic. The error states you define
imply what checks should be implemented in the game logic. Do not worry
about having too many checks impacting performance; most validation
will be turned off in release builds. These checks are mainly for
testing and debugging.

## Board

The board type represents most of the game state, so this is usually the
most complex type to design. Use an extremely memory-efficient
representation — it will be copied for every node in the MCTS tree. If
needed, use a struct and pack information as tightly as possible.

## Static contract

The `Game` concept also requires three `static constexpr` constants and a
`PolicyIndex(action)` member. Decide and document each:

- `kHistoryLookback` — how many past states the network input needs.
  Markov games (chess, Go, tic-tac-toe) declare 0; Atari-style games
  with frame stacking declare 4–8.
- `kPolicySize` — cardinality of the full action space, ignoring
  legality. Equal to the size of the network's fixed-size policy head.
- `kMaxRounds` — `std::optional<uint32_t>` self-play hard cap. Use
  `std::nullopt` for genuinely unbounded games. If set, `IsOver()` must
  return `true` once `CurrentRound() >= *kMaxRounds`.
- `PolicyIndex(action)` — bijection from `action_t` to a slot in
  `[0, kPolicySize)`. The default policy serializer/deserializer use
  this to scatter and gather masked policy values.

## Mutation primitives

`ApplyActionInPlace(action)` advances the state in place;
`UndoLastAction()` reverses the most recent apply. Both must be
allocation-free — they sit on the MCTS hot path and are called once per
expanded node in a rollout. Plan the private state so undo can chain
back through the deepest MCTS rollout the engine will run; a single-slot
`last_action_` is rarely enough. A small fixed-size action history
(`std::array<action_t, MAX_DEPTH>`) is the typical pattern.

## Length and format

Although these documents are not skills, follow LLM skill best practices
where useful.

Keep the markdown document under 200 lines, with each line under 80
characters. These documents will be used by LLMs, so keep them concise
and avoid wasting context. Begin each document with a 1–2 sentence
summary of the game, followed by a table of contents under a
`## Content` header.

Some designs are too complex to fit under the length limit. In that
case, focus on the core design in the main document and link to
separate documents for details. Create a new directory under
[memory](../../../memory/) named `game_design_details` and store new
markdown files there using snake_case.md filenames, with one file per
specific aspect of the design. These detail documents do not need a ToC
header unless they exceed 100 lines.

Do not nest document references, i.e., `game_design.md` can reference
other documents, but those documents should not reference any other
documents. If a detail document needs to point elsewhere, either
include that content directly or move the reference up to
`game_design.md`.

## Specific tips for different types of games

**Card games**: see [card_games.md](./card_games.md).
