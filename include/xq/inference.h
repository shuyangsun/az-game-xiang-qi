#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_INFERENCE_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_INFERENCE_H_

#include <span>
#include <vector>

#include "alpha-zero-api/augmenter.h"
#include "alpha-zero-api/policy_output.h"
#include "include/xq/game.h"

namespace az::game::xq {

/**
 * @brief Inference-time augmenter: expand one game state into N
 * equivalent states, then aggregate the per-variant `Evaluation`s back
 * into a single `Evaluation` for the original state.
 *
 * TODO(TASK-INFERENCE-IMPL): tailor this docstring once the augmentation
 * set is finalized. Document how variant probabilities are
 * rotated/mirrored back to the original action space, and how multiple
 * variant values are combined (mean is the typical choice).
 */
class XqInferenceAugmenter
    : public ::az::game::api::IInferenceAugmenter<XqGame> {
 public:
  XqInferenceAugmenter() = default;
  ~XqInferenceAugmenter() override = default;

  /**
   * @brief Expand the input game state into all augmented variants.
   *
   * Convention: `result[0]` is the identity (`game` itself), so a game
   * with no useful symmetry can return a one-element vector and
   * `Interpret` becomes effectively the identity. The order matches the
   * `XqAugmentation` enum.
   */
  [[nodiscard]] std::vector<XqGame> Augment(
      const XqGame& game) const noexcept final;

  /**
   * @brief Combine per-variant evaluations into a single `Evaluation`
   * for the original game state.
   *
   * TODO(TASK-INFERENCE-IMPL): document the inverse mapping from each
   * variant action back to the original action, and the aggregation
   * strategy (mean / max / weighted, etc.).
   *
   * `augmented` and `evaluations` are aligned: `evaluations[i]`
   * corresponds to `augmented[i]`. The returned `Evaluation`'s
   * probabilities must align with `original.ValidActionsInto(...)`; the
   * implementation is responsible for inverting whatever symmetry it
   * applied.
   */
  [[nodiscard]] ::az::game::api::Evaluation Interpret(
      const XqGame& original, std::span<const XqGame> augmented,
      std::span<const ::az::game::api::Evaluation> evaluations)
      const noexcept final;
};

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_INFERENCE_H_
