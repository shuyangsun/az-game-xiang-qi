---
name: adding-unittests
description: Adding unittests based on the checklist and specific task at hand.
---

# Adding Unittests

## Scripts

Run every command below from the **project root**. Do not `cd` into the skill
folder. Invoke by the full project-root-relative path starting with
`.agents/...`, NOT `./scripts/...` or `./script_name.sh`.

- [`find_fr.sh`](./scripts/find_fr.sh) — `.agents/skills/adding-unittests/scripts/find_fr.sh SEARCH_DIR FR-ID`

## Workflow

Read [memory/unittest_checklists.md](../../../memory/unittest_checklists.md) to
get a list of functionalities that need to be tested. Do NOT try to add all
tests at once, focus on the current task at hand from
[memory/tasks.md](../../../memory/tasks.md). Think about which functionality
is related to the task at hand, and only add tests for those functionalities.

For example, if you are asked to work on game constructors test, you do not need
to add tests related to game string conversion, game actions, or serializer
constructors. Only focus on the game constructor in this scenario.

The `unittest_checklists.md` may be an ordered list or bullet points of markdown
checklist items. Do NOT mark any item as complete, that will be done in a future
process. Instead, each checklist item should have a unique `FR-` identifier, add
this identifier to the test name in CamelCase format. For example, `FR-001`
would be `TEST(GameConstructors, FR_001_NoArgument)`, or `FR-CONSTRUCTOR-NO-ARG`
would be `TEST(GameConstructors, FR_CONSTRUCTOR_NO_ARG)`. Use `find_fr.sh` (e.g.
`find_fr.sh tests/unit FR-001`) to find test files with a specific `FR`
identifier.

There may be existing placeholder tests created from the Cookiecutter template,
change their names to match the `FR-` identifiers using the convention mentioned
above.

Implement the test logic based on the unittest checklist item description, and
use [game_rules.md](../../../memory/game_rules.md) as the source of truth
reference. Do not skip tests anymore, we need real test logic in place now.

Just make sure tests build correctly using the `building-with-cmake` skill, they
do NOT need to pass, and it is okay if they crash as the implementation is
likely not done yet. We are using TDD approach when adding tests.
Review the code using the `reviewing-code` skill after you are done.
