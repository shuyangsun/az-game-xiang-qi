#include "include/xq/train.h"

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
  // TODO(TASK-TRAIN-IMPL): generate every augmented training example.
  // The policy probabilities must be permuted to match the augmented
  // game's `ValidActions()` ordering so the network learns
  // symmetry-equivariant policies. `target.z` is preserved unchanged.

  std::vector<XqGame> augmented = internal::AugmentAll(game);

  std::vector<std::pair<XqGame, ::az::game::api::TrainingTarget>> result;
  result.reserve(augmented.size());

  for (auto&& aug_game : augmented) {
    // TODO(TASK-TRAIN-IMPL): permute `target.pi` so probabilities stay
    // aligned with `aug_game.ValidActions()`. Copying `target` unchanged
    // (as below) trains the network to be augmentation-invariant instead
    // of equivariant — wrong, but lets the placeholder compile.
    result.emplace_back(std::move(aug_game), target);
  }

  return result;
}

}  // namespace az::game::xq
