#include "include/xq/serializer.h"

#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "alpha-zero-api/ring_buffer.h"
#include "include/xq/game.h"

namespace az::game::xq {

std::vector<float> XqSerializer::SerializeCurrentState(
    const XqGame& game,
    ::az::game::api::RingBufferView<XqGame> history) const noexcept {
  // TODO(TASK-SERIALIZER-IMPL): encode `game` (and, if non-Markov,
  // `history`) into a fixed-size float vector. Reserve the exact final
  // size before pushing to avoid reallocations. Document the layout in
  // the header.

  return {};
}

std::vector<float> XqSerializer::SerializePolicyOutput(
    const XqGame& game,
    const ::az::game::api::TrainingTarget& target) const noexcept {
  // TODO(TASK-SERIALIZER-IMPL): encode `target` into a fixed-size float
  // vector. Convention: result[0] = target.z; the rest is the
  // `kPolicySize`-wide policy distribution scattered via
  // `game.PolicyIndex(action)` so invalid actions get zero mass.

  return {};
}

}  // namespace az::game::xq
