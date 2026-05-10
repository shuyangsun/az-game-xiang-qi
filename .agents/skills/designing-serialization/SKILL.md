---
name: designing-serialization
description: Design the state-input and policy-output serializers for a
transformer-based AlphaZero network.
---

# Designing the Xiang Qi serializer

Write a design doc covering both halves of
[serializer.h](../../../include/xq/serializer.h)
and the matching
[deserializer.h](../../../include/xq/deserializer.h)
in [memory/serialization_design.md](../../../memory/serialization_design.md).
Do not implement anything in `.cc` files yet; that is what
`SERIALIZER-IMPL` and `DESERIALIZER-IMPL` are for. The output of this
skill is the contract those tasks build against.

Ground every decision in
[memory/game_rules.md](../../../memory/game_rules.md) and
[memory/game_design.md](../../../memory/game_design.md). Read
[memory/api_contract.md](../../../memory/api_contract.md),
[memory/defaults.md](../../../memory/defaults.md), and
[memory/history_lookback.md](../../../memory/history_lookback.md) before
proposing any layout — they pin the signatures, the engine-owned
history view, and the off-the-shelf scatter/gather defaults you may or
may not want to reuse.

## Architectural premise

The downstream policy/value network is a **transformer**, not a stack
of 2D convolutions like the original AlphaGo paper. That changes the
serializer design space in concrete ways:

- The natural input unit is a **token** (a small float vector), not a
  pixel-with-channels. A position, a card, a piece, an action history
  entry, or a per-player summary can each be one token.
- Spatial structure is not implicit in tensor layout — you encode it
  inside each token (board coords, region tag, time step, owning
  player) so attention can recover it.
- Variable-length state is natural for transformers, but the API still
  hands a flat `std::vector<float>` to the network. Pad to a fixed
  `(max_seq_len, d_token)` and reserve one feature dimension as a
  validity / mask flag the network can read.
- Embedding lookups beat one-hot. Pass small categorical IDs as
  floats; let the network learn the embedding. This trims the input
  size dramatically for large vocabularies (cards, piece types).

Resist copying CNN-era encodings (`C × H × W` one-hot planes) wholesale
— they waste tokens and make positional reasoning the network's
problem instead of the encoder's.

## State input — `SerializeCurrentState`

Decide and document, in order:

1. **Token inventory.** Enumerate every kind of token the network
   sees: e.g. `[CLS]`-style global summary, board-cell tokens,
   private-hand tokens, public-pile tokens, per-player metadata,
   history-step tokens, padding. Give each a unique `token_type` ID.
2. **Per-token feature schema.** A fixed-width float vector per
   token. Typical fields: `token_type`, `position_id` (or
   `(row, col)`), `time_step`, `owner_player`, payload IDs (piece
   type, card rank/suit, etc.), `is_valid`. Keep `d_token` small;
   the network projects it up.
3. **Sequence layout.** Order tokens so the layout is deterministic
   across reachable states. Document the order explicitly — the
   deserializer never sees state input, but training-data tooling
   and unit tests do.
4. **Padding.** `max_seq_len` is fixed across all reachable states.
   Pad shorter sequences with a dedicated padding-token ID and set
   `is_valid = 0`. The flat output length is exactly
   `max_seq_len * d_token`.
5. **Player perspective.** Use `game.CanonicalBoard()` so the network
   always sees "me" vs "opponent" rather than absolute player IDs.
   This is what makes a single network usable from every seat.
6. **Hidden information.** The serializer runs from a single player's
   point of view (the current player). Encode only what that player
   can observe; mask or omit private opponent state.
7. **History.** The engine passes a
   `RingBufferView<XqGame>` of size up to
   `kHistoryLookback`. Either prepend per-step token blocks with a
   `time_step` feature, or fold history into delta features on the
   current-state tokens. Markov games (`kHistoryLookback == 0`) skip
   this entirely.

## Action / policy output — `SerializePolicyOutput`

The `Game` concept fixes a `kPolicySize` slot count and a
`PolicyIndex(action)` bijection (see
[api_contract.md](../../../memory/api_contract.md)). Whatever the
network looks like internally, the **serializer's flat float layout
must match what the deserializer reads.** Decide:

1. **Layout.** Default is
   `[z, p_0, p_1, ..., p_{kPolicySize-1}]` — what
   `DefaultPolicyOutputSerializer<G>` produces (see
   [defaults.md](../../../memory/defaults.md)). Use it unless you
   have a concrete reason. Reasons to roll your own:
   - separate value and policy tensors,
   - distributional value head (`C51` / quantile),
   - per-action auxiliary targets (e.g. predicted value per move).
2. **Probability vs logit.** Targets are probabilities (MCTS visit
   distribution). The network may emit logits and the deserializer
   may apply softmax. Document who normalizes — pick one place.
3. **Mask handling.** Slots not in `game.ValidActions()` get `0` in
   the target so the network learns the mask implicitly. The
   deserializer typically also zeros illegal slots before softmax /
   renormalization. Keep both ends consistent.
4. **Pointer-style heads (optional).** Transformers can emit a
   policy by attending over input action tokens rather than indexing
   a fixed slot. AlphaZero's static `kPolicySize` contract still
   requires a flat scatter for training labels — if you go this
   route, document the mapping from "attended action token i" back
   to `PolicyIndex(action)` so the bijection is preserved.
5. **Action factorization (optional).** Composite actions (move
   piece A from X to Y) tempt a multi-token decomposition. Doing so
   breaks the single-distribution AlphaZero contract; only consider
   it with a custom training loop. Default: keep one slot per
   atomic action.

## Deserializer mirror

The deserializer's job is the inverse of `SerializePolicyOutput`:
read the flat float vector, gather slots for `game.ValidActions()` via
`game.PolicyIndex(action)`, and produce an `Evaluation`. Spell out:

- expected `output.size()` (e.g. `kPolicySize + 1` for the default
  layout),
- which slot is `value`,
- whether logits are softmaxed here or upstream,
- the error type (`std::string` vs the game's typed `error_t`).

If the input encoding ever changes, the deserializer almost certainly
does not — it only touches output. Note this explicitly so future you
does not over-edit.

## Augmentation interaction

If the project was generated with `augmenter = yes`, a transformer
encoding is **not** automatically augmentation-equivariant the way a
CNN with rotated planes is. Either:

- bake position invariance into the token features (so a rotated
  board produces a permuted token sequence the network sees as the
  same set), and rely on attention's permutation invariance, or
- materialize symmetries through the training augmenter as usual
  (see
  [augmentation_strategy.md](../../../memory/augmentation_strategy.md)).

Pick one and say so in the design doc — the training loop's behavior
depends on it.

## Length and format

Keep the doc under 200 lines, lines under 80 characters. Begin with a
1–2 sentence summary plus a `## Contents` ToC, like
[writing-game-header-design-doc](../writing-game-header-design-doc/SKILL.md)
prescribes. Show the concrete per-token feature schema and the flat
output layout as small code blocks or tables — the implementer will
copy them verbatim.

If the layout is too large to fit, push detail into
`memory/serialization_design_details/` using `snake_case.md` files,
one per topic (e.g. `token_schema.md`, `history_encoding.md`). Do
not nest references: `serialization_design.md` may link out, but the
detail files should not link to each other.
