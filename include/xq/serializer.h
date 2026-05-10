#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_SERIALIZER_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_SERIALIZER_H_

#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "alpha-zero-api/ring_buffer.h"
#include "alpha-zero-api/serializer.h"
#include "include/xq/game.h"

namespace az::game::xq {

/**
 * @brief Serializes Xiang Qi game state and training
 * target to fixed-size float vectors suitable for neural network
 * input/output.
 *
 * TODO(TASK-SERIALIZER-IMPL): tailor this docstring to be Xiang Qi
 * specific. Describe the input encoding (channels, shape, normalization)
 * and the output encoding (action layout via `PolicyIndex`, value-slot
 * placement, padding strategy for invalid actions).
 *
 * Implements both `::az::game::api::IGameSerializer<XqGame>`
 * and `::az::game::api::IPolicyOutputSerializer<XqGame>`
 * to keep the encoding logic for the neural network input and the training
 * labels together.
 */
class XqSerializer : public ::az::game::api::IGameSerializer<XqGame>,
                     public ::az::game::api::IPolicyOutputSerializer<XqGame> {
 public:
  XqSerializer() = default;
  ~XqSerializer() override = default;

  /**
   * @brief Serialize the current game state into a fixed-size float
   * vector that is passed as input to the policy/value neural network.
   *
   * TODO(TASK-SERIALIZER-IMPL): tailor this docstring once the input
   * encoding is decided. Common encodings include one-hot per piece type,
   * channels for "to-move" / castling rights / move counters, etc.
   * For non-Markov games, `history` provides the most-recent
   * `kHistoryLookback` past states (index 0 = newest); use them if the
   * input encoding requires temporal context.
   *
   * The returned vector MUST be of fixed length across all reachable game
   * states because it is consumed by the neural network as a tensor.
   *
   * @param game Current game state.
   * @param history Engine-owned window over recent past states; size is
   * bounded by `XqGame::kHistoryLookback`. `game`
   * itself is not in the view.
   * @return std::vector<float> Neural network input tensor (flattened).
   */
  [[nodiscard]] std::vector<float> SerializeCurrentState(
      const XqGame& game,
      ::az::game::api::RingBufferView<XqGame> history) const noexcept final;

  /**
   * @brief Serialize a `TrainingTarget` (z, π) into a fixed-size float
   * vector suitable for use as a network training target.
   *
   * TODO(TASK-SERIALIZER-IMPL): tailor this docstring once the output
   * encoding is decided. The canonical layout puts `target.z` in slot 0
   * and the `kPolicySize`-wide policy distribution starting at slot 1,
   * scattered via `game.PolicyIndex(action)`.
   *
   * The returned vector MUST be of fixed length across all reachable game
   * states. Probability mass for actions not in `game.ValidActionsInto(...)`
   * should be zero so the network learns the valid-action mask
   * implicitly.
   *
   * `target.pi[i]` corresponds to the i-th entry written by
   * `game.ValidActionsInto(buf)` into the caller-owned buffer. The
   * implementation is responsible for scattering those values into the
   * fixed-size policy slot via `game.PolicyIndex(action)`.
   *
   * @param game Game state the target was produced for.
   * @param target Training target (`z`, `pi`).
   * @return std::vector<float> Neural network output tensor (flattened).
   */
  [[nodiscard]] std::vector<float> SerializePolicyOutput(
      const XqGame& game,
      const ::az::game::api::TrainingTarget& target) const noexcept final;
};

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_SERIALIZER_H_
