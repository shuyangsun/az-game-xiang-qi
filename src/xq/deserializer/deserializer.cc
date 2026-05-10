#include "include/xq/deserializer.h"

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "include/xq/game.h"

namespace az::game::xq {

XqResult<::az::game::api::Evaluation> XqDeserializer::Deserialize(
    const XqGame& game, std::span<const float> output) const noexcept {
  // Layout (mirrors XqSerializer::SerializePolicyOutput):
  //   output[0]                   = value scalar
  //   output[1 + PolicyIndex(a)]  = prior for action `a`
  // Total length: kPolicySize + 1.
  constexpr std::size_t kExpected = XqGame::kPolicySize + 1;
  if (output.size() != kExpected) {
    return std::unexpected(XqError::kInvalidPolicyOutputSize);
  }

  const std::vector<XqA> actions = game.ValidActions();
  std::vector<float> probs;
  probs.reserve(actions.size());
  for (const XqA& a : actions) {
    probs.push_back(output[1 + game.PolicyIndex(a)]);
  }
  return ::az::game::api::Evaluation{output.front(), std::move(probs)};
}

}  // namespace az::game::xq
