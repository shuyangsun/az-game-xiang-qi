# Board encoding

## Cell layout

- 90 cells, row-major `r * 9 + c` for `r in [0, 9]`, `c in [0, 8]`.
- `XqB = std::array<int8_t, 90>`. One signed byte per cell.

## Cell value codes

| Code | Meaning           |
| ---- | ----------------- |
| 0    | empty             |
| +1   | Red General       |
| +2   | Red Advisor       |
| +3   | Red Elephant      |
| +4   | Red Horse         |
| +5   | Red Chariot       |
| +6   | Red Cannon        |
| +7   | Red Soldier       |
| -1   | Black General     |
| -2   | Black Advisor     |
| -3   | Black Elephant    |
| -4   | Black Horse       |
| -5   | Black Chariot     |
| -6   | Black Cannon      |
| -7   | Black Soldier     |

Sign distinguishes color; magnitude indexes piece type. The
magnitudes match the order in `game_rules_details/pieces.md`.

## Starting position

Construction sets:

- Row 0 (Red back): `[+5, +4, +3, +2, +1, +2, +3, +4, +5]`
- Row 2: cannons at cols 1 and 7 (`+6`)
- Row 3: soldiers at cols 0, 2, 4, 6, 8 (`+7`)
- Row 6: soldiers at cols 0, 2, 4, 6, 8 (`-7`)
- Row 7: cannons at cols 1 and 7 (`-6`)
- Row 9 (Black back): `[-5, -4, -3, -2, -1, -2, -3, -4, -5]`
- Everything else `0`.

## CanonicalBoard

`CanonicalBoard()` returns the board from the current player's
perspective:

- If `current_player_ == Player1` (Red): return `board_` as-is.
- If `current_player_ == Player2` (Black): return a copy with
  every non-zero cell negated and every cell rotated 180 degrees
  via `(r, c) -> (9 - r, 8 - c)`. The player to move always sees
  their pieces as positive and advancing toward increasing rows.

## Network input encoding (15 planes of 9x10)

The state serializer emits 15 planes laid out
`plane * 90 + r * 9 + c`, total 1350 floats.

- Planes 0–6: current player's pieces, one plane per piece type
  (General, Advisor, Elephant, Horse, Chariot, Cannon, Soldier).
  `1.0` if a piece of that type sits on the cell, else `0.0`.
- Planes 7–13: opponent's pieces, same per-type layout.
- Plane 14: side-to-move plane. All `1.0` if Red to move,
  all `0.0` if Black to move.

The encoding is computed on top of `CanonicalBoard()` so the
"current player vs opponent" plane split is automatic.

## Memory profile

- `XqB`: 90 bytes.
- 15 planes are computed on demand by the serializer; they are
  not stored in the game state.
