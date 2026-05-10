# Xiang Qi Game Header Design

TODO(TASK-DESIGN-DOC): write design doc for the game header.

The interactive REPL in `src/main.cc` (see [main_binary.md](./main_binary.md))
imposes a few cross-cutting constraints on this design — write them in
explicitly so later tasks have something to point at:

- **Player** must be `bool`, an integral type, or a class with a
  `std::ostream& operator<<` overload. The REPL uses these to print
  `"Player N"` lines.
- **Action** must support a string round-trip: `ActionFromString` must
  accept every string that `ActionToString` returns. `ActionToString`
  must be deterministic and return a distinct string per distinct
  action.
- **Board** must produce a `BoardReadableString` that a human can
  meaningfully follow turn-by-turn during self-play.
