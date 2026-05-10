#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_TRAIN_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_TRAIN_H_

#include <utility>
#include <vector>

#include "alpha-zero-api/augmenter.h"
#include "alpha-zero-api/policy_output.h"
#include "include/xq/game.h"

namespace az::game::xq {

/**
 * @brief Training-time augmenter: turn one (game, target) example into N
 * equivalent training tuples by applying every supported augmentation.
 *
 * TODO(TASK-TRAIN-IMPL): tailor this docstring once the augmentation set
 * is finalized. The training augmenter must transform the policy
 * probabilities alongside the board so the network learns
 * symmetry-equivariant policies. `target.z` is preserved unchanged
 * because board symmetries are score-preserving.
 */
class XqTrainingAugmenter : public ::az::game::api::ITrainingAugmenter<XqGame> {
 public:
  XqTrainingAugmenter() = default;
  ~XqTrainingAugmenter() override = default;

  /**
   * @brief Generate every augmented `(game, target)` training tuple from
   * a single example.
   *
   * Each returned pair contains a XqGame variant and
   * a `TrainingTarget` whose `pi[i]` corresponds to that variant's
   * the i-th entry written by `ValidActionsInto`. `target.z` is preserved unchanged.
   *
   * Convention: result includes the identity (typically the first
   * element); callers should not rely on order.
   */
  [[nodiscard]] std::vector<std::pair<XqGame, ::az::game::api::TrainingTarget>>
  Augment(const XqGame& game,
          const ::az::game::api::TrainingTarget& target) const noexcept final;
};

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_TRAIN_H_
