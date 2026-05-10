#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_DESERIALIZER_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_DESERIALIZER_H_

#include <span>

#include "alpha-zero-api/deserializer.h"
#include "alpha-zero-api/policy_output.h"
#include "include/xq/game.h"

namespace az::game::xq {

/**
 * @brief Deserializes the raw float output from the policy/value neural
 * network into an `::az::game::api::Evaluation`.
 *
 * TODO(TASK-DESERIALIZER-IMPL): tailor this docstring to describe how the
 * neural network output is laid out and how it is mapped back onto valid
 * actions for Xiang Qi.
 *
 * The deserializer must mirror the layout produced by
 * `XqSerializer::SerializePolicyOutput`. The
 * `Evaluation::probabilities` vector returned must have the same length
 * as `game.ValidActionsInto(...)`. Implementations typically gather the masked
 * subset of policy slots via `game.PolicyIndex(action)`.
 */
class XqDeserializer
    : public ::az::game::api::IPolicyOutputDeserializer<XqGame, XqError> {
 public:
  XqDeserializer() = default;
  ~XqDeserializer() override = default;

  /**
   * @brief Convert raw neural network output into an `Evaluation`
   * restricted to `game.ValidActionsInto(...)`.
   *
   * TODO(TASK-DESERIALIZER-IMPL): document the expected output size, the
   * action -> index mapping (typically via `game.PolicyIndex`), and the
   * renormalization strategy (e.g., softmax over masked logits).
   *
   * @param game Game state used to produce the network input.
   * @param output Raw network output tensor (flat). Callers must
   * up-convert FP16/BF16 outputs to FP32 before invoking the
   * deserializer.
   * @return XqResult<::az::game::api::Evaluation>
   *         `Evaluation` on success, error on malformed input.
   */
  [[nodiscard]] XqResult<::az::game::api::Evaluation> Deserialize(
      const XqGame& game, std::span<const float> output) const noexcept final;
};

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_DESERIALIZER_H_
