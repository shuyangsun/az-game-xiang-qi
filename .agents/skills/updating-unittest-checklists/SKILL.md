---
name: updating-unittest-checklists
description: Updating unittest checklists based on the game rules document.
---

# Updating unittest checklists

Update unittest checklists [memory/unittest_checklists.md](../../../memory/unittest_checklists.md)
based on [memory/game_rules.md](../../../memory/game_rules.md) and its
references.

Read the game rules document and convert each bullet point to one or more bullet
point checklist items. Group checklist items by the sections in the game rules
document. Each rule item should be phrased as a functional software requirement,
with `FR-` prefix and a unique number or meaningful short identifier.

The checklist may have already been populated before, in that case, you should
review the game rules document and identify any missing or inconsistent items.
If there are any, show the human a numbered list of all of them and ask them to
confirm or select which ones to update.

Do NOT remove the `TODO(TASK-REVIEW-GAME-FR-COV): ...` line, keep it at the top
of the file, it is needed for other tasks.

## Examples

- [guan_dan.md](./examples/guan_dan.md), from [this rule doc](../writing-game-rule-doc/examples/guan_dan.md).
