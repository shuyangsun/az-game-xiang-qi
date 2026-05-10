#include "include/xq/train.h"

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
  // with the variant's own ValidActions() ordering. target.z is
  // preserved unchanged because board symmetries are score-preserving.
  const std::vector<XqA> orig_actions = game.ValidActions();
  std::vector<XqGame> variants = internal::AugmentAll(game);

  std::vector<std::pair<XqGame, ::az::game::api::TrainingTarget>> result;
  result.reserve(variants.size());

  // Lookup: original-frame PolicyIndex -> position in target.pi.
  std::vector<std::size_t> orig_slot(XqGame::kPolicySize,
                                      orig_actions.size());
  for (std::size_t i = 0; i < orig_actions.size(); ++i) {
    orig_slot[game.PolicyIndex(orig_actions[i])] = i;
  }

  for (std::size_t i = 0; i < variants.size(); ++i) {
    const internal::XqAugmentation sym =
        static_cast<internal::XqAugmentation>(i);
    XqGame variant = variants[i];
    const std::vector<XqA> v_actions = variant.ValidActions();

    std::vector<float> permuted(v_actions.size(), 0.0F);
    for (std::size_t j = 0; j < v_actions.size(); ++j) {
      const XqA orig_action =
          internal::InverseTransformAction(v_actions[j], sym);
      const std::size_t pidx = game.PolicyIndex(orig_action);
      if (pidx >= orig_slot.size()) continue;
      const std::size_t src = orig_slot[pidx];
      if (src < target.pi.size()) {
        permuted[j] = target.pi[src];
      }
    }

    result.emplace_back(std::move(variant),
                        ::az::game::api::TrainingTarget{target.z,
                                                        std::move(permuted)});
  }
  return result;
}

}  // namespace az::game::xq
