# Action encoding

## Type

```cpp
struct XqA {
  uint8_t from;  // source cell, 0..89, or 90 for the sentinel
  uint8_t to;    // destination cell, 0..89, or 90 for the sentinel
};
```

`XqA` is 2 bytes, trivially copyable. Cell encoding matches
`board_encoding.md`: cell `c` corresponds to row `c / 9`, column
`c % 9`.

## Sentinel "no action"

- `XqA{90, 90}` is the sentinel. `ValidActions()` never returns
  it. It exists so default-constructed `action_history_` slots
  never collide with real moves and so undo can detect "nothing
  to undo".

## Equality and hashing

- Game state never compares `XqA` directly on the hot path; the
  REPL compares actions by their `ActionToString` form.
  Operator overloads are not required by the API contract, so
  none are added.

## PolicyIndex

```cpp
constexpr std::size_t PolicyIndex(const XqA& a) const noexcept {
  return static_cast<std::size_t>(a.from) * 90 + a.to;
}
```

- Range: `[0, 8100)`.
- Bijection over `XqA{from, to}` where each is in `[0, 89]`.
- Slots whose `from`/`to` correspond to physically impossible
  moves (e.g., source empty, destination off-board) are still
  valid policy slots; they are simply masked out by
  `ValidActions()` and stay `0` in the policy serializer.

## ActionToString format

Use a compact human-readable notation:

```text
<from_col><from_row>-<to_col><to_row>
```

- `<col>` is `'a'..'i'` (cols 0..8).
- `<row>` is `'0'..'9'` (rows 0..9, matching the engine row
  index — row 0 is Red's back rank).
- Example: Red opens with horse from `(0, 1)` to `(2, 2)` →
  `"b0-c2"`.

The format is deterministic and round-trips through
`ActionFromString`, which trims whitespace, lower-cases the
column letters, and rejects malformed input with the appropriate
`XqError`.

## ValidActions ordering

Iterate cells `c` in `[0, 90)`. For each cell whose piece
belongs to the current player, generate all reachable
destinations in a piece-type-specific deterministic order
(e.g., orthogonal directions in N, E, S, W order; diagonal in
NE, SE, SW, NW order). Push each `XqA{from, to}` into the
result vector. After every candidate is generated, filter out
those that leave own General in check or violate Flying General.

The exact iteration order is part of the contract: re-running
`ValidActions()` on the same state returns the same vector.
