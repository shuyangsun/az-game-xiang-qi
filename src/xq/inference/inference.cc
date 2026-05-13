#include "include/xq/inference.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <utility>
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
  // its position in `original.ValidActionsInto()`.
  std::array<XqA, XqGame::kMaxLegalActions> orig_actions{};
  const size_t orig_count = original.ValidActionsInto(orig_actions);
  std::vector<size_t> slot_for_policy_index(XqGame::kPolicySize, orig_count);
  for (size_t i = 0; i < orig_count; ++i) {
    slot_for_policy_index[original.PolicyIndex(orig_actions[i])] = i;
  }

  // Aggregate value (mean) and probabilities (mean of inverse-mapped
  // per-variant probability vectors).
  std::vector<float> probs(orig_count, 0.0F);
  float value_sum = 0.0F;
  size_t variant_count = 0;
  std::array<XqA, XqGame::kMaxLegalActions> v_actions{};
  const size_t n = std::min(augmented.size(), evaluations.size());
  for (size_t i = 0; i < n; ++i) {
    const internal::XqAugmentation sym =
        static_cast<internal::XqAugmentation>(i);
    const size_t v_count = augmented[i].ValidActionsInto(v_actions);
    const ::az::game::api::Evaluation& eval = evaluations[i];
    const size_t m = std::min(v_count, eval.probabilities.size());
    for (size_t j = 0; j < m; ++j) {
      const XqA orig_action =
          internal::InverseTransformAction(v_actions[j], sym);
      const size_t pidx = original.PolicyIndex(orig_action);
      if (pidx >= slot_for_policy_index.size()) continue;
      const size_t slot = slot_for_policy_index[pidx];
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
