---
name: working-on-tasks
description: Identify remaining tasks and implement the Xiang Qi
game. Use when the user gives you a vague instruction to work on something.
---

# AlphaZero Xiang Qi Implementation Agent

## Scripts

Run every command below from the **project root**. Do not `cd` into the skill
folder. Invoke by the full project-root-relative path starting with
`.agents/...`, NOT `./scripts/...` or `./script_name.sh`.

- [`next_task.sh`](./scripts/next_task.sh) — `.agents/skills/working-on-tasks/scripts/next_task.sh TASKS_MD`
- [`find_task.sh`](./scripts/find_task.sh) — `.agents/skills/working-on-tasks/scripts/find_task.sh SEARCH_DIR TASK-ID`

## Pick a task

- If the user assigned specific task(s), work on those. If an assigned task is
  already marked `[x]`, confirm with the user before redoing it.
- Otherwise, run `next_task.sh memory/tasks.md` to get the next incomplete task
  as a short snippet. **Do NOT read the full
  [memory/tasks.md](../../../memory/tasks.md)** — it is large and pollutes the
  context window.

## Work the task

- Run `find_task.sh . TASK-ID` (e.g. `find_task.sh . UPDATE-README`) to locate
  all TODOs for the task. Every TODO must be resolved before the task is
  complete.
- [memory/constitution.md](../../../memory/constitution.md) is non-negotiable.
- Match model strength to task difficulty (fast/cheap for `easy`, strong and
  longer-thinking for `hard`) when the harness allows it.
- Use whichever other skills fit the task.

## Finish the task

- Rerun `find_task.sh` to confirm no TODOs remain.
- Flip the task's `[ ]` to `[x]` on its line in
  [memory/tasks.md](../../../memory/tasks.md). `next_task.sh` prints the exact
  line number — edit just that line; don't read the whole file.
- **Exception:** "HRR" stands for human review required. If `HRR = Yes`, leave
  it unchecked and ask the user to review and mark it complete.

## Continue or stop?

Context window hygiene matters: finishing one task then stopping keeps future
sessions fast and focused. Decide what to do next using this order:

1. **Multiple assigned tasks.** Continue to the next assigned task. Compact
   between tasks if the conversation is getting long.
2. **Start of a new session and `/compact` is available.** Ask the user _once_
   whether they'd like you to auto-compact after each task and continue to the
   next. If they agree, use the command below after marking the task complete,
   then rerun `next_task.sh` and proceed.
3. **Otherwise.** Stop after one task. Report completion and suggest starting a
   new conversation for the next task.

Use `/compact` with this phrasing so only the essentials survive:

```text
/compact summarize high-level progress, keep very brief descriptions of key
decisions and reasoning, remove all other details.
```
