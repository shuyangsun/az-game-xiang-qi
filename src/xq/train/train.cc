#include "include/xq/train.h"

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "include/xq/augmentation.h"
#include "include/xq/game.h"

namespace az::game::xq {

std::vector<std::pair<XqGame, ::az::game::api::TrainingTarget>>
XqTrainingAugmenter::Augment(
    const XqGame& game,
    const ::az::game::api::TrainingTarget& target) const noexcept {
  // For each augmented variant, permute target.pi so it stays aligned
  // with the variant's own ValidActionsInto() ordering. target.z is
  // preserved unchanged because board symmetries are score-preserving.
  std::array<XqA, XqGame::kMaxLegalActions> orig_actions{};
  const size_t orig_count = game.ValidActionsInto(orig_actions);
  std::vector<XqGame> variants = internal::AugmentAll(game);

  std::vector<std::pair<XqGame, ::az::game::api::TrainingTarget>> result;
  result.reserve(variants.size());

  // Lookup: original-frame PolicyIndex -> position in target.pi.
  std::vector<size_t> orig_slot(XqGame::kPolicySize, orig_count);
  for (size_t i = 0; i < orig_count; ++i) {
    orig_slot[game.PolicyIndex(orig_actions[i])] = i;
  }

  std::array<XqA, XqGame::kMaxLegalActions> v_actions{};
  for (size_t i = 0; i < variants.size(); ++i) {
    const internal::XqAugmentation sym =
        static_cast<internal::XqAugmentation>(i);
    XqGame variant = variants[i];
    const size_t v_count = variant.ValidActionsInto(v_actions);

    std::vector<float> permuted(v_count, 0.0F);
    for (size_t j = 0; j < v_count; ++j) {
      const XqA orig_action =
          internal::InverseTransformAction(v_actions[j], sym);
      const size_t pidx = game.PolicyIndex(orig_action);
      if (pidx >= orig_slot.size()) continue;
      const size_t src = orig_slot[pidx];
      if (src < target.pi.size()) {
        permuted[j] = target.pi[src];
      }
    }

    result.emplace_back(std::move(variant), ::az::game::api::TrainingTarget{
                                                target.z, std::move(permuted)});
  }
  return result;
}

}  // namespace az::game::xq
