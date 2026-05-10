# API Defaults

What ships in `alpha-zero-api/defaults/` and when each piece is useful
for Xiang Qi. Read after [api_contract.md](./api_contract.md).

## `defaults/game.h` — board / action / player aliases

| Alias | Type | Use when |
| --- | --- | --- |
| `Standard1DBoard<SZ>` | `std::array<int8_t, SZ>` | small fixed-size 1D board |
| `Standard1DLargeBoard` | `std::vector<int8_t>` | large or runtime-sized 1D board |
| `Standard2DBoard<R, C>` | `std::array<std::array<int8_t, C>, R>` | small fixed-size grid |
| `Standard2DLargeBoard` | `std::vector<std::vector<int8_t>>` | large or runtime-sized grid |
| `Action1D` | `int16_t` | single-index action (e.g. cell N) |
| `Action2D` | `struct { uint16_t row, col; }` | grid coordinate action |
| `BinaryPlayer` | `bool` (with `Player1 = false`, `Player2 = true`) | two-player games |

These are convenience type aliases — using them is purely a memory and
ergonomics choice. Custom `board_t` / `action_t` / `player_t` types are
fine and often required (large action spaces, packed bitboards,
multi-player, non-grid topologies).

The `BinaryPlayer = bool` alias has a special-case in the REPL: the
`PlayerString` helper prints `"Player 1"` / `"Player 2"` for `bool`,
`"Player <n>"` for other integral types, and falls back to
`operator<<(std::ostream&, const Player&)` for class types.

## `defaults/serializer.h` — `DefaultPolicyOutputSerializer<G>`

Canonical scatter from `TrainingTarget` to the network's fixed-size
policy output:

```text
out[0]                      = target.z
out[1 + PolicyIndex(a)]     = target.pi[i]   for actions[i] == a
out[1 + (other)]            = 0
```

Layout is `[z, p_0, p_1, ..., p_{kPolicySize-1}]`, total length
`kPolicySize + 1`.

**Use it when** `target.z` in slot 0 followed by a flat policy vector
matches your network's output head. **Skip it when** you need a
different layout (e.g. value head before policy head as separate
tensors, or you want softmax baked in here rather than in the
deserializer).

## `defaults/deserializer.h` — `DefaultPolicyOutputDeserializer<G>`

Mirror of the default serializer:

- Validates `output.size() == kPolicySize + 1`.
- Returns `Evaluation{output[0], gather of output[1 + PolicyIndex(a)] for
  a in ValidActions()}`.
- Returns the gathered values **verbatim** — no implicit softmax or
  renormalization. If the network emits logits, compose your own
  softmax (the tic-tac-toe example does this in its custom
  deserializer).
- Hardcodes `std::string` as `error_t`. Games with typed errors should
  implement `IPolicyOutputDeserializer<G, MyError>` directly.

## When to use defaults vs. custom

For Xiang Qi the input encoding (`SerializeCurrentState`)
is almost always custom — there is no default for it because every
game's network input layout is different. The two policy-output
defaults above are usually a good starting point; switch to custom when:

- Your network expects a different layout (separate value/policy
  tensors, value head with multiple scalars, distributional value head).
- Your deserializer needs to softmax-normalize, temperature-scale, or
  apply an illegal-action mask differently from the default.
- You want a typed `error_t` rather than `std::string` for deserializer
  failures.

Document the chosen layout in `serializer.h` so the deserializer (and
any downstream training pipeline) stays in sync.
