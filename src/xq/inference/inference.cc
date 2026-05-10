#include "include/xq/inference.h"

#include <span>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "include/xq/augmentation.h"
#include "include/xq/game.h"

namespace az::game::xq {

std::vector<XqGame> XqInferenceAugmenter::Augment(
    const XqGame& game) const noexcept {
  // TODO(TASK-INFERENCE-IMPL): typically just delegates to
  // internal::AugmentAll. Override only if inference needs a different
  // (usually smaller) augmentation set than training.
  return internal::AugmentAll(game);
}

::az::game::api::Evaluation XqInferenceAugmenter::Interpret(
    const XqGame& original, std::span<const XqGame> augmented,
    std::span<const ::az::game::api::Evaluation> evaluations) const noexcept {
  // TODO(TASK-INFERENCE-IMPL): for each (augmented[i], evaluations[i]),
  // map every per-variant action probability back to its original-frame
  // action (typically by composing the inverse symmetry with
  // `original.PolicyIndex`) and accumulate. Average values across
  // variants.

  return ::az::game::api::Evaluation{0.0f, std::vector<float>{}};
}

}  // namespace az::game::xq
