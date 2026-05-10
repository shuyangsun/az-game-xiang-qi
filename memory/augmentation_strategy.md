# Augmentation Strategy

How to design `XqAugmentation`,
`internal::AugmentAll`, and the inference / training augmenters for
Xiang Qi. Read after [api_contract.md](./api_contract.md)
and the symmetry section of [game_design.md](./game_design.md).

## What augmentation buys you

The neural network is trained to be **equivariant** under board
symmetries: a position rotated by 90° produces a policy rotated by
90°, and the same value. By materializing every symmetry-equivalent
position as an extra training example, the network sees `N×` more
data per self-play game and converges faster. At inference time, the
engine evaluates every symmetry-equivalent position, then averages the
returned `Evaluation`s — this denoises the network and is essentially
free relative to MCTS cost.

If Xiang Qi has no exploitable board symmetry (most
card games, asymmetric games, games with intrinsic orientation),
return only the identity. The augmenters then degenerate into the
identity transform.

## The `XqAugmentation` enum

In [augmentation.h](../include/xq/augmentation.h):

```cpp
enum class XqAugmentation : uint8_t {
  kOriginal = 0,
  // ... one entry per supported symmetry ...
};
```

Conventions:

- `kOriginal = 0` — identity, always present.
- Order matches `internal::AugmentAll(game)` output:
  `result[i] == AugmentAll(game)[i]` corresponds to
  `static_cast<XqAugmentation>(i)`.
- Underlying type `uint8_t` — augmentation index doubles as a key in
  hash maps and the inverse-action lookup.

Common symmetry sets:

| Game shape | Symmetry group | Order |
| --- | --- | --- |
| Square board, full dihedral (tic-tac-toe, Go) | D4 | 8 (4 rotations × 2 mirrors) |
| Rectangular board (Connect Four, Othello on `n×m` with `n≠m`) | mirror only | 2 |
| Hex / triangular board | depends on shape | 6 or 12 |
| Asymmetric board (chess, shogi) | none useful for the network | 1 (identity only) |

The tic-tac-toe reference implementation lists 12 entries — D4 has
order 8, but it keeps four duplicate mirror-rotations to preserve
historical behavior. Avoid that pattern in new games unless you know
why; duplicates inflate per-position network calls without adding
information.

## Inverse action map

`Interpret` and the training augmenter both need to map an action
expressed in an augmented coordinate frame back to the original frame:

```cpp
[[nodiscard]] XqA InverseTransformAction(
    const XqA& augmented_action,
    XqAugmentation augmentation) noexcept;
```

Implement and declare this alongside `AugmentAll` so both augmenter
classes can share it. The math is the inverse of whatever transform
`AugmentAll` applied for that enum value — `RotateClockwise` ⇄
`RotateCounterclockwise`, mirrors are self-inverse, etc.

## Inference augmenter aggregation

`Interpret(original, augmented, evaluations)` walks each variant,
inverts the symmetry on each per-variant action probability, and
accumulates into a vector indexed by `original.ValidActions()`. The
canonical aggregation is **mean** for both value and probabilities:

```text
combined.value          = mean over i of evaluations[i].value
combined.probabilities  = mean over i of inverse(evaluations[i].probs)
```

Keep the inverse map seeded by `original.PolicyIndex(action)` so
augmented actions land in the right slot regardless of how each
variant orders its `ValidActions()`. The tic-tac-toe inference
implementation upstream is a good template.

## Training augmenter equivariance

`ITrainingAugmenter::Augment(game, target)` must permute `target.pi`
**alongside** the augmented board. Returning `target` unchanged trains
the network to be augmentation-invariant rather than equivariant —
wrong, and the placeholder in `train.cc` does this on purpose so the
template compiles before TASK-TRAIN-IMPL is done.

`target.z` is preserved unchanged — board symmetries are
score-preserving.

## Tests to keep honest

The unittest checklist in [unittest_checklists.md](./unittest_checklists.md)
covers the inference / training augmenter contract from the API side.
Add game-specific FRs for any symmetry your design relies on (e.g.
"applying RotateClockwise four times yields the original board").
