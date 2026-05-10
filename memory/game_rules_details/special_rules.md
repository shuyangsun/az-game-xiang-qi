# Xiang Qi Special Rules

## Check

- The General is "in check" if the opposing side could capture
  it by some legal-piece move on the next half-move (ignoring
  the Flying General check on that hypothetical move).
- A move that leaves the moving side's General in check is
  illegal — even if the moving side was not previously in check.
- Capturing the General is not a real move; the engine instead
  ends the game by the standard checkmate rule below.

## Flying General (face-to-face Generals)

- The two Generals may never sit on the same file with the
  intervening intersections all empty.
- This is enforced as a constraint on every move: a move is
  illegal if, after the move, the two Generals would face each
  other directly along an empty file.
- Practical consequence: a Cannon, Chariot, or any piece pinned
  on the General's file may not step away if doing so would
  expose the face-to-face position.

## Checkmate

- The side to move is in check.
- The side to move has no legal move (every candidate move
  either does not address the check or would leave its own
  General in check / face-to-face).
- The side to move loses; the opponent wins.

## Stalemate (no legal moves)

- The side to move has zero legal moves.
- If it is also in check, this is checkmate (above).
- If it is not in check, by Asian Xiang Qi rules the side to
  move still loses. (Western chess scores this as a draw; this
  implementation follows the Asian convention.)

## Repetition

- A "position" includes: piece placement on every cell **and**
  the side to move.
- If the same position recurs three times across the game, the
  game ends in a draw.
- Strict Asian rules forbid perpetual check and perpetual chase
  (the offending side loses). This engine intentionally treats
  any threefold repetition as a draw, because:
  - Perpetual-chase detection requires reasoning about the
    *intent* of the chasing piece (which piece is chased, can
    the chased piece trade, etc.) — too much policy for an
    engine that the AlphaZero network is meant to discover.
  - Threefold-repetition draw is unambiguous, deterministic, and
    sufficient for self-play termination.
- Document this deviation in the rule doc and in any test that
  exercises repetition behavior.

## Move limit (engine-imposed draw)

- The engine sets `kMaxRounds` (a hard ply cap). When
  `CurrentRound() >= *kMaxRounds`, `IsOver()` returns true and
  the score is `0` for both sides.
- This cap is not a Xiang Qi rule per se; it exists so
  pathological self-play games (e.g., early-iteration networks
  shuffling pieces forever) still terminate.

## Captures and scoring

- A capture removes the captured piece from the board. There is
  no "promotion" or material score in the standard rules. The
  engine returns `+1 / -1 / 0` based purely on win / loss /
  draw, as documented in [../game_rules.md](../game_rules.md).
