# Xiang Qi Pieces

Each side has 16 pieces of 7 distinct types. "Friendly" means
same color, "enemy" means opposite color, "empty" means the
intersection holds no piece.

A move is valid if:

1. The destination is reachable by the piece's movement rule.
2. The destination is empty or holds an enemy piece (capture).
3. After the move, the moving side's General is not in check.
4. The move does not violate the Flying General rule (see
   [special_rules.md](./special_rules.md)).

## General (帥 Red / 將 Black)

- Moves exactly 1 step horizontally or vertically.
- Confined to the palace (3x3 region of own side).
- Subject to the Flying General rule: at the end of the move,
  the two Generals must not face each other on the same file
  with no pieces between them.

## Advisor (仕 Red / 士 Black)

- Moves exactly 1 step diagonally.
- Confined to the palace.
- Reaches only 5 distinct points of the palace: the four corners
  and the center.

## Elephant (相 Red / 象 Black)

- Moves exactly 2 steps diagonally — i.e., from `(r, c)` to
  `(r+2, c+2)` or one of the other three diagonal-2 directions.
- Cannot cross the river: Red Elephant stays on rows 0-4, Black
  Elephant stays on rows 5-9.
- "Blocking the eye": the intermediate point `(r+1, c+1)` (or
  the equivalent for the other directions) must be empty. If it
  holds any piece, the Elephant cannot make that move.

## Horse (馬 / 傌)

- Moves in an "L": one step orthogonally, then one step
  diagonally outward — equivalent to a chess knight's 8 target
  offsets.
- "Hobbling the horse": the piece on the orthogonal step blocks
  the move. For example, a horse at `(r, c)` moving to
  `(r+2, c+1)` is blocked if `(r+1, c)` is occupied; the move
  to `(r+1, c+2)` is blocked if `(r, c+1)` is occupied.
- The Horse may capture at the destination.

## Chariot (車 / 俥)

- Moves any number of steps along a rank or file.
- May not jump pieces — every intervening intersection must be
  empty.
- May capture by landing on an enemy piece in the line of sight.

## Cannon (砲 / 炮)

- Movement without capture: identical to Chariot — any number of
  empty intersections horizontally or vertically.
- Capture move: must jump exactly one piece (called the
  "screen") between itself and the target. The screen may be
  friendly or enemy. There must be exactly one piece between
  source and target; the target itself must be enemy.
- A Cannon never captures without a screen, and never has more
  than one screen.

## Soldier (兵 Red / 卒 Black)

- Before crossing the river: moves exactly 1 step forward.
  - Red forward = `row + 1`. Black forward = `row - 1`.
- After crossing the river: moves 1 step forward, left, or
  right. Sideways moves are along the same rank.
- Never moves backward, never moves diagonally.
- Captures by displacement (no special capture rule).
- Soldiers do **not** promote to anything else.

## Summary table

| Piece    | Moves                         | Range | Captures by           | Confined?          |
| -------- | ----------------------------- | ----- | --------------------- | ------------------ |
| General  | 1 orthogonal                  | 1     | displacement          | own palace         |
| Advisor  | 1 diagonal                    | 1     | displacement          | own palace         |
| Elephant | 2 diagonal, eye must be empty | 2     | displacement          | own side of river  |
| Horse    | L-shape, leg must be empty    | 2     | displacement          | none               |
| Chariot  | orthogonal, no jumping        | any   | displacement          | none               |
| Cannon   | orthogonal, jump 1 to capture | any   | jump exactly 1 screen | none               |
| Soldier  | 1 forward (or sideways past   | 1     | displacement          | forward / no back  |
|          | river)                        |       |                       |                    |
