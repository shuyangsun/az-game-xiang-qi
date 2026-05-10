# Repetition tracking

## Goal

Detect threefold repetition (same board + side-to-move three
times) so `IsOver()` can return `true` for the draw case.

## Approach: Zobrist hashing + per-ply hash log

- Maintain a running 64-bit Zobrist hash of the position. The
  hash combines:
  - One key per (cell, piece-code) pair: 90 cells × 14 piece
    codes = 1260 random keys.
  - One key for "Black to move" (XOR'd in when it's Black's
    turn).
- On `ApplyActionInPlace`:
  - XOR out the moving piece on `from`, XOR in on `to`.
  - XOR out any captured piece on `to`.
  - XOR the side-to-move key.
  - Push the resulting hash into `position_history_[round_]`.
- On `UndoLastAction`:
  - Reverse the XORs (Zobrist is its own inverse).
  - Effectively, pop `position_history_`.

`position_history_` is `std::array<uint64_t, 300>` — sized to
`kMaxRounds`. Total cost: 2.4 KiB per game state.

## Detection

`IsOver()` (and similar termination checks) compute occurrence
of the **current** position hash by scanning
`position_history_[0..round_-1]`. If the count is `>= 3`, the
game is a draw. Linear scan is O(`round_`); for `round_ <= 300`
this is negligible compared to move generation.

## Hash-only repetition is approximate

A 64-bit Zobrist hash collides with probability ≈ 2^-64 per
comparison; for any reachable Xiang Qi game length this is
effectively zero. We accept the residual risk and document it.
A defensive tightening (also compare board planes when hashes
match) is left as a follow-up.

## Constructor seeding

The Zobrist key table is a `constexpr std::array<uint64_t, ...>`
populated from a small deterministic xorshift PRNG seeded with a
fixed constant. Same keys every program run, no platform
variance.

## Interaction with snapshot constructor

The snapshot constructor (used by augmenters) builds the
position hash from scratch by XORing every occupied cell. It
sets `round_` to the supplied value but seeds
`position_history_[round_-1]` with the current hash only if
`last_action.has_value()`; older history is left zero. Augmented
games are thus not safe to use for repetition-driven termination
detection — augmenters operate on single positions, not on
playthroughs. Document that limitation explicitly in the
augmentation tests.
