---
name: writing-game-rule-doc
description: Write the game rules document to inform game design.
---

# Writing game rule document

Write a game rule document in [memory/game_rules.md](../../../memory/game_rules.md)
as if you are explaining the game to a new player.

The document is intended for a human reader, so they can collaborate with you on
clarifying the game rules. Once the document is finalized, it will be used as a
reference for writing other design docs and implementing the game logic. Do not
assume the collaborator has any prior knowledge about the game, and assume they
do not have any coding experience. You must make your best judgement on striking
a balance between human-readable and informing API implementation.

## Human friendly while API informed

Do not write the doc as if you are writing a requirement doc for software
engineers, the reader likely does not have any technical background. However,
the end goal of this document is still to inform the API implementation.

You should use your best judgement to include and clarify game rules that may
impact the API type design and implementation, but make it easy to understand
for non-technical readers (e.g., a middle school student interested in arts).

## Key aspects to cover

The Xiang Qi specific C++ API header is
[include/xq/game.h](../../../include/xq/game.h),
which implements the generic AlphaZero API header. The header may have some
pre-existing type definitions, but they are likely placeholders if there are
TODO comments around them. Use the `finding-alpha-zero-api` skill if the
game-specific API does not provide enough information.

Read the Xiang Qi API header to understand what information is
needed. In the rule book, make sure to think through and explain these core
aspects:

- What is the smallest unit that can be treated as a complete game? (informs
  constructor design) Some games, such as Texas hold'em, are long-running but
  consist of multiple smaller games. Each smaller game can still count as a
  complete game, just with different initialization parameters. It is fine to
  treat each round as a complete game if decisions from earlier rounds do not
  directly affect later rounds, and any carryover can be fully captured in the
  next round's initialization parameters.
- What information is needed at the beginning of the game? (informs constructor
  design)
- When is the game considered to be over? (informs termination logic design)
- What is the winning or losing condition? (informs reward design)
- Is the game team-based? Does the reward matter only for the team or individual
  player score should also be optimized? What if they conflict with each other?
  (informs reward design)
- What are the possible actions a player can take? (informs action design)
- Are there special actions at the beginning or end of the game? (informs
  action design)

Cover all of the above, but remember that this is not a design document. It is
for human readers with no technical background.

## Format and length

Although these documents are not skills, follow LLM skill best practices
when applicable.

Use different sections for different aspects of the game rules. Each section
should have a very high-level overview of what it covers, and the rest should be
bullet points or numbered markdown checklists to break down each rule. This
format helps LLMs to write unittest checklists.

Example:

```markdown
# Game Rules

## Overview

Briefly describe the game in 1-2 sentences.

## Number of players

- Rule 1
- ...

## Winning condition

- Rule 1
- Rule 2
- ...
```

Keep the markdown document under 200 lines, with each line under 80 characters.
These documents will be used by LLMs, so conciseness is key, don't pollute the
context window. At the beginning of the document, include a brief summary of the
game in 1-2 sentences, followed by a ToC with `## Content` header.

Some games are too complex to fit under the length limit. In that case, focus on
including the core rules in the main document, and link to other documents for
specifics. Create a new directory under [memory](../../../memory/) called
`game_rules_details` to store new markdown files with snake_case.md names, each
covering a specific aspect of the game rules. These documents do not need a ToC
header, unless they themselves are more than 100 lines.

Do not nest document references, i.e., `game_rules.md` can reference other
documents, but those documents should not reference any other documents. If you
need to reference other documents in detailed documents, consider either
including the content in the detailed document, or referencing the new document
in `game_rules.md` instead.

## Specific tips for different types of games

**Card games**: see [card_games.md](./card_games.md).
