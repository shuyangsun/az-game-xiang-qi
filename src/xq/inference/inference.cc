#include "include/xq/inference.h"

#include <cstddef>
#include <span>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "include/xq/augmentation.h"
#include "include/xq/game.h"

namespace az::game::xq {

std::vector<XqGame> XqInferenceAugmenter::Augment(
    const XqGame& game) const noexcept {
  return internal::AugmentAll(game);
}

::az::game::api::Evaluation XqInferenceAugmenter::Interpret(
    const XqGame& original, std::span<const XqGame> augmented,
    std::span<const ::az::game::api::Evaluation> evaluations) const noexcept {
  // Build a lookup from each original-frame action's PolicyIndex to
  // its position in `original.ValidActions()`.
  const std::vector<XqA> orig_actions = original.ValidActions();
  std::vector<std::size_t> slot_for_policy_index(XqGame::kPolicySize,
                                                  orig_actions.size());
  for (std::size_t i = 0; i < orig_actions.size(); ++i) {
    slot_for_policy_index[original.PolicyIndex(orig_actions[i])] = i;
  }

  // Aggregate value (mean) and probabilities (mean of inverse-mapped
  // per-variant probability vectors).
  std::vector<float> probs(orig_actions.size(), 0.0F);
  float value_sum = 0.0F;
  std::size_t variant_count = 0;
  const std::size_t n = std::min(augmented.size(), evaluations.size());
  for (std::size_t i = 0; i < n; ++i) {
    const internal::XqAugmentation sym =
        static_cast<internal::XqAugmentation>(i);
    const std::vector<XqA> v_actions = augmented[i].ValidActions();
    const ::az::game::api::Evaluation& eval = evaluations[i];
    const std::size_t m = std::min(v_actions.size(), eval.probabilities.size());
    for (std::size_t j = 0; j < m; ++j) {
      const XqA orig_action =
          internal::InverseTransformAction(v_actions[j], sym);
      const std::size_t pidx = original.PolicyIndex(orig_action);
      if (pidx >= slot_for_policy_index.size()) continue;
      const std::size_t slot = slot_for_policy_index[pidx];
      if (slot >= probs.size()) continue;
      probs[slot] += eval.probabilities[j];
    }
    value_sum += eval.value;
    ++variant_count;
  }
  if (variant_count > 0) {
    const float inv = 1.0F / static_cast<float>(variant_count);
    for (float& p : probs) p *= inv;
    value_sum *= inv;
  }
  return ::az::game::api::Evaluation{value_sum, std::move(probs)};
}

}  // namespace az::game::xq
