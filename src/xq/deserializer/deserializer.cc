#include "include/xq/deserializer.h"

#include <span>
#include <utility>

#include "alpha-zero-api/policy_output.h"
#include "include/xq/game.h"

namespace az::game::xq {

XqResult<::az::game::api::Evaluation> XqDeserializer::Deserialize(
    const XqGame& game, std::span<const float> output) const noexcept {
  // TODO(TASK-DESERIALIZER-IMPL): map raw network output to an
  // `Evaluation` over `game.ValidActions()`. Validate `output.size()`
  // against the layout produced by
  // XqSerializer::SerializePolicyOutput, gather
  // masked policy values via `game.PolicyIndex`, softmax-normalize, and
  // return std::unexpected(XqError::...) on
  // mismatch.

  return std::unexpected(XqError::kNotImplemented);
}

}  // namespace az::game::xq
