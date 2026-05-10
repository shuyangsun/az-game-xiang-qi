# Xiang Qi Termination Reference

This file enumerates the terminal conditions the implementation
must detect and the scores it must return. It is a precise
reference for `IsOver()`, `GetScore()`, and the unit tests that
cover them.

## Terminal conditions (ordered by priority)

A position is terminal as soon as the **first** of these is
true. Implementations should evaluate them in this order:

1. **Checkmate**
   - Side to move is in check.
   - Side to move has zero legal moves.
   - Side to move **loses**: `GetScore(side_to_move) == -1`,
     `GetScore(opponent) == +1`.

2. **Stalemate** (no legal moves, not in check)
   - Side to move has zero legal moves.
   - Side to move is **not** in check.
   - Side to move **loses** (Asian rules):
     `GetScore(side_to_move) == -1`,
     `GetScore(opponent) == +1`.

3. **Threefold repetition**
   - The current position (board + side to move) has occurred
     three times across this game's history.
   - Game is a **draw**: `GetScore(player) == 0` for both.

4. **Move limit**
   - `CurrentRound() >= *kMaxRounds`.
   - Game is a **draw**: `GetScore(player) == 0` for both.

If none of the above is true, `IsOver()` returns `false`.

## Score semantics during play

- While `IsOver()` is `false`, `GetScore` is allowed to return
  `0` for both players. Self-play training only consumes
  `GetScore` after `IsOver()` is `true`, so the in-progress
  value is unconstrained — but `0/0` is the most predictable
  default and is what this implementation returns.

## Edge cases the implementation must handle

- A move that captures the enemy General is impossible by
  construction: such a move would leave the opponent in
  "check + no recovery", which is checkmate, and the prior turn
  would already have terminated. Defensive code should still
  treat "General missing" as `IsOver() == true` with the side
  whose General is missing scored `-1`.
- A move that exposes one's own General to check is illegal and
  must not appear in `ValidActions()`. This includes Flying
  General exposures.
- Repetition counters reset on undo. `UndoLastAction()` must
  restore the position-occurrence map exactly so that
  `IsOver()` after `Apply -> Undo` matches the pre-apply state.
