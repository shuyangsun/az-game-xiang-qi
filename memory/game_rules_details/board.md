# Xiang Qi Board

## Geometry

- 9 vertical lines (files), 10 horizontal lines (ranks).
- 90 grid intersections — pieces sit on intersections, not
  inside squares.
- Engine coordinates: `row` in `[0, 9]`, `col` in `[0, 8]`.
  - Row 0 is Red's home rank, row 9 is Black's home rank.
  - Col 0 is the leftmost file as seen from Red, col 8 the
    rightmost.
  - Cell index `r * 9 + c` is in `[0, 89]`.
- "Forward" depends on color: Red moves toward higher `row`
  numbers, Black toward lower `row` numbers.

## Special regions

- **River** — between rows 4 and 5. A piece is "on Red's side"
  if `row <= 4`, "on Black's side" if `row >= 5`. Some pieces
  may not cross it (Elephant), some change behavior after
  crossing (Soldier).
- **Palace** (also "fortress") — a 3x3 region per side:
  - Red palace: rows 0-2, cols 3-5.
  - Black palace: rows 7-9, cols 3-5.
  - Generals and Advisors must stay inside their palace.
- **Diagonals inside the palace** — only the General-palace
  diagonals are walked by Advisors; they connect the palace
  corners and the palace center.

## Starting position

Red side (rows 0-3):

```text
row 3:  .  .  .  .  .  .  .  .  .   <- soldiers row (cols 0,2,4,6,8)
row 2:  .  C  .  .  .  .  .  C  .   <- cannon row (cols 1,7)
row 1:  .  .  .  .  .  .  .  .  .   (empty)
row 0:  R  H  E  A  G  A  E  H  R   <- back rank
        0  1  2  3  4  5  6  7  8
```

Where `R = Chariot`, `H = Horse`, `E = Elephant`, `A = Advisor`,
`G = General`, `C = Cannon`. Soldiers `S` sit on row 3 at cols
0, 2, 4, 6, 8.

Black side (rows 6-9) is the mirror image: back rank on row 9
in the same column order, cannons on row 7 at cols 1 and 7,
soldiers on row 6 at cols 0, 2, 4, 6, 8.

Total pieces per side at start: 1 General, 2 Advisors,
2 Elephants, 2 Horses, 2 Chariots, 2 Cannons, 5 Soldiers — 16
each, 32 in all.

## Symmetry

- The board is **left-right symmetric** along the central file
  (col 4). Mirroring all pieces across that file yields a legal,
  equivalent position. This is the only board symmetry useful
  for AlphaZero augmentation.
- The board is **not** vertically symmetric (the river divides
  Red's and Black's territory and piece zones differ).
- Rotations are **not** symmetries (river / palace orientation
  is fixed).
