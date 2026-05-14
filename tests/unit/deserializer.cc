#include "include/xq/deserializer.h"

#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "gtest/gtest.h"
#include "include/xq/game.h"
#include "include/xq/serializer.h"
#include "tests/unit/valid_actions.h"

namespace az::game::xq {
namespace {

using ::az::game::api::CompactPolicyOutputBlob;
using ::az::game::api::CompactPolicyTargetBlob;
using ::az::game::api::Evaluation;
using ::az::game::api::TrainingTarget;

constexpr size_t kPolicyVectorLen = XqGame::kPolicySize + 1;

bool ApproxEqual(float a, float b) noexcept {
  constexpr float kAbs = 1e-5F;
  constexpr float kRel = 1e-4F;
  const float diff = std::fabs(a - b);
  return diff <= kAbs || diff <= kRel * std::max(std::fabs(a), std::fabs(b));
}

TEST(Deserializer, FR_DES_WRONG_SIZE_RejectsTooShort) {
  const XqGame game;
  const XqDeserializer deserializer;
  const std::vector<float> short_output(kPolicyVectorLen - 1, 0.0F);
  const auto result = deserializer.Deserialize(game, short_output);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), XqError::kInvalidPolicyOutputSize);
}

TEST(Deserializer, FR_DES_WRONG_SIZE_RejectsTooLong) {
  const XqGame game;
  const XqDeserializer deserializer;
  const std::vector<float> long_output(kPolicyVectorLen + 1, 0.0F);
  const auto result = deserializer.Deserialize(game, long_output);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), XqError::kInvalidPolicyOutputSize);
}

TEST(Deserializer, FR_DES_WRONG_SIZE_RejectsEmpty) {
  const XqGame game;
  const XqDeserializer deserializer;
  const std::vector<float> empty_output;
  const auto result = deserializer.Deserialize(game, empty_output);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), XqError::kInvalidPolicyOutputSize);
}

TEST(Deserializer, FR_DES_PARALLEL_SizeMatchesValidActions) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  const std::vector<float> output(kPolicyVectorLen, 0.0F);
  const XqDeserializer deserializer;
  const auto result = deserializer.Deserialize(game, output);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->probabilities.size(), actions.size());
}

TEST(Deserializer, FR_DES_VALUE_PASSTHROUGH_OutputZeroIsValue) {
  const XqGame game;
  std::vector<float> output(kPolicyVectorLen, 0.0F);
  output[0] = -0.75F;
  const XqDeserializer deserializer;
  const auto result = deserializer.Deserialize(game, output);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->value, -0.75F);
}

TEST(Deserializer, FR_DES_PARALLEL_GathersByPolicyIndex) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  // Write distinct values into each valid policy slot; expect the
  // deserializer to gather them in `ValidActions()` order.
  std::vector<float> output(kPolicyVectorLen, 0.0F);
  for (size_t i = 0; i < actions.size(); ++i) {
    output[1 + game.PolicyIndex(game.CanonicalAction(actions[i]))] =
        static_cast<float>(i + 1);
  }
  const XqDeserializer deserializer;
  const auto result = deserializer.Deserialize(game, output);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->probabilities.size(), actions.size());
  for (size_t i = 0; i < actions.size(); ++i) {
    EXPECT_FLOAT_EQ(result->probabilities[i], static_cast<float>(i + 1))
        << "Mismatch at slot " << i;
  }
}

// Black variant: the deserializer must read from canonical-frame slots,
// not live-frame slots. Writing distinct values into the
// `PolicyIndex(CanonicalAction(a))` slots and gathering must recover
// them in `ValidActions()` order.
TEST(Deserializer, FR_DES_PARALLEL_GathersByCanonicalSlotForBlack) {
  const XqGame game(kBlack);
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> output(kPolicyVectorLen, 0.0F);
  for (size_t i = 0; i < actions.size(); ++i) {
    output[1 + game.PolicyIndex(game.CanonicalAction(actions[i]))] =
        static_cast<float>(i + 1);
  }
  const XqDeserializer deserializer;
  const auto result = deserializer.Deserialize(game, output);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->probabilities.size(), actions.size());
  for (size_t i = 0; i < actions.size(); ++i) {
    EXPECT_FLOAT_EQ(result->probabilities[i], static_cast<float>(i + 1))
        << "Mismatch at slot " << i;
  }
}

TEST(Deserializer, FR_DES_ROUNDTRIP_RecoversValueAndProbabilities) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi;
  pi.reserve(actions.size());
  // Build a probability distribution with distinct values so the
  // roundtrip catches any swap or drop.
  float running_sum = 0.0F;
  for (size_t i = 0; i < actions.size(); ++i) {
    pi.push_back(1.0F / static_cast<float>(actions.size() + i));
    running_sum += pi.back();
  }
  for (float& v : pi) {
    v /= running_sum;
  }
  const TrainingTarget target{0.123F, pi};

  const XqSerializer serializer;
  const std::vector<float> nn_out =
      serializer.SerializePolicyOutput(game, target);
  const XqDeserializer deserializer;
  const auto recovered = deserializer.Deserialize(game, nn_out);
  ASSERT_TRUE(recovered.has_value());
  EXPECT_TRUE(ApproxEqual(recovered->value, target.z));
  ASSERT_EQ(recovered->probabilities.size(), pi.size());
  for (size_t i = 0; i < pi.size(); ++i) {
    EXPECT_TRUE(ApproxEqual(recovered->probabilities[i], pi[i]))
        << "Probability mismatch at index " << i << ": "
        << recovered->probabilities[i] << " != " << pi[i];
  }
}

TEST(CompactDeserializer, FR_DES_COMPACT_WRONG_SIZE_RejectsMismatchedRows) {
  const XqGame game;
  const XqCompactDeserializer deserializer;
  const std::vector<size_t> indices(1, 0);
  const std::vector<float> values;
  const CompactPolicyOutputBlob output{0.0F, std::span<const size_t>{indices},
                                       std::span<const float>{values}};
  const XqResult<Evaluation> result = deserializer.Deserialize(game, output);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), XqError::kInvalidPolicyOutputSize);
}

TEST(CompactDeserializer, FR_DES_COMPACT_WRONG_SIZE_RejectsMissingActions) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  const XqCompactDeserializer deserializer;
  const std::vector<size_t> indices;
  const std::vector<float> values;
  const CompactPolicyOutputBlob output{0.0F, std::span<const size_t>{indices},
                                       std::span<const float>{values}};
  const XqResult<Evaluation> result = deserializer.Deserialize(game, output);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), XqError::kInvalidPolicyOutputSize);
}

TEST(CompactDeserializer, FR_DES_COMPACT_ROUNDTRIP_PaddedSortedPolicy) {
  const XqGame game(kBlack);
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi;
  pi.reserve(actions.size());
  float running_sum = 0.0F;
  for (size_t i = 0; i < actions.size(); ++i) {
    pi.push_back(1.0F / static_cast<float>(actions.size() + i));
    running_sum += pi.back();
  }
  for (float& v : pi) {
    v /= running_sum;
  }

  const TrainingTarget target{-0.25F, pi};
  const XqCompactSerializer serializer;
  const CompactPolicyTargetBlob blob =
      serializer.SerializePolicyOutput(game, target);
  const CompactPolicyOutputBlob output{
      blob.value, std::span<const size_t>{blob.legal_indices},
      std::span<const float>{blob.values}};

  const XqCompactDeserializer deserializer;
  const XqResult<Evaluation> recovered = deserializer.Deserialize(game, output);
  ASSERT_TRUE(recovered.has_value());
  EXPECT_TRUE(ApproxEqual(recovered->value, target.z));
  ASSERT_EQ(recovered->probabilities.size(), pi.size());
  for (size_t i = 0; i < pi.size(); ++i) {
    EXPECT_TRUE(ApproxEqual(recovered->probabilities[i], pi[i]))
        << "Probability mismatch at index " << i << ": "
        << recovered->probabilities[i] << " != " << pi[i];
  }
}

TEST(CompactDeserializer, FR_DES_COMPACT_ROUNDTRIP_UnpaddedPolicy) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi;
  pi.reserve(actions.size());
  for (size_t i = 0; i < actions.size(); ++i) {
    pi.push_back(static_cast<float>(i + 1));
  }

  const TrainingTarget target{0.75F, pi};
  const XqCompactSerializer serializer;
  const CompactPolicyTargetBlob blob =
      serializer.SerializePolicyOutput(game, target);
  const CompactPolicyOutputBlob output{
      blob.value,
      std::span<const size_t>{blob.legal_indices.data(), blob.count},
      std::span<const float>{blob.values.data(), blob.count}};

  const XqCompactDeserializer deserializer;
  const XqResult<Evaluation> recovered = deserializer.Deserialize(game, output);
  ASSERT_TRUE(recovered.has_value());
  EXPECT_FLOAT_EQ(recovered->value, target.z);
  ASSERT_EQ(recovered->probabilities.size(), pi.size());
  for (size_t i = 0; i < pi.size(); ++i) {
    EXPECT_FLOAT_EQ(recovered->probabilities[i], pi[i]);
  }
}

}  // namespace
}  // namespace az::game::xq
