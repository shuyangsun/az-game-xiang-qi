---
name: reviewing-fr-test-coverage
description: Review functional requirements test coverage based on unittest checklists.
---

# Reviewing Functional Requirements Test Coverage

## Scripts

Run every command below from the **project root**. Invoke by the full
project-root-relative path starting with `.agents/...`, NOT `./scripts/...` or
`./script_name.sh`. (This skill borrows `find_fr.sh` from the `adding-unittests`
skill.)

- [`find_fr.sh`](../adding-unittests/scripts/find_fr.sh) — `.agents/skills/adding-unittests/scripts/find_fr.sh SEARCH_DIR FR-ID`

## Workflow

Tests were added to [tests/unit/](../../../tests/unit/) based on the unittest
checklists in [memory/unittest_checklists.md](../../../memory/unittest_checklists.md).
Use `find_fr.sh` (e.g. `find_fr.sh tests/unit FR-001`) to find test files with
a specific `FR` identifier. Mark the checklist items as complete in
`unittest_checklists.md` if there are tests implemented for those functional
requirements.
