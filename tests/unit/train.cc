#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "gtest/gtest.h"
#include "include/xq/augmentation.h"
#include "include/xq/game.h"
#include "include/xq/train.h"

namespace az::game::xq {
namespace {

using ::az::game::api::TrainingTarget;

bool ApproxEqual(float a, float b) noexcept {
  constexpr float kAbs = 1e-5F;
  constexpr float kRel = 1e-4F;
  const float diff = std::fabs(a - b);
  return diff <= kAbs ||
         diff <= kRel * std::max(std::fabs(a), std::fabs(b));
}

TrainingTarget UniformTarget(const XqGame& game, float z) {
  const std::vector<XqA> actions = game.ValidActions();
  std::vector<float> pi(actions.size(),
                        actions.empty()
                            ? 0.0F
                            : 1.0F / static_cast<float>(actions.size()));
  return TrainingTarget{z, std::move(pi)};
}

TEST(TrainingAugmenter, FR_TRAIN_AUG_ONE_PER_OnePairPerAugmentation) {
  const XqGame game;
  const XqTrainingAugmenter trainer;
  const TrainingTarget target = UniformTarget(game, 0.5F);
  const auto pairs = trainer.Augment(game, target);
  EXPECT_EQ(pairs.size(), internal::kNumAugmentations);
}

TEST(TrainingAugmenter, FR_TRAIN_AUG_Z_PRESERVED_AcrossEveryPair) {
  const XqGame game;
  const XqTrainingAugmenter trainer;
  const TrainingTarget target = UniformTarget(game, -0.25F);
  const auto pairs = trainer.Augment(game, target);
  for (const auto& [aug_game, aug_target] : pairs) {
    EXPECT_TRUE(ApproxEqual(aug_target.z, target.z))
        << "Symmetries are score-preserving; z must be untouched.";
  }
}

TEST(TrainingAugmenter,
     FR_TRAIN_AUG_PI_PERMUTED_AlignsWithAugmentedValidActions) {
  const XqGame game;
  const std::vector<XqA> orig_actions = game.ValidActions();
  if (orig_actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }

  // Build a non-uniform target so a wrong permutation is detectable.
  std::vector<float> pi;
  pi.reserve(orig_actions.size());
  float total = 0.0F;
  for (std::size_t i = 0; i < orig_actions.size(); ++i) {
    const float v = 1.0F / static_cast<float>(i + 2);
    pi.push_back(v);
    total += v;
  }
  for (float& v : pi) v /= total;
  const TrainingTarget target{0.0F, pi};

  const XqTrainingAugmenter trainer;
  const auto pairs = trainer.Augment(game, target);
  for (const auto& [aug_game, aug_target] : pairs) {
    // pi length must match the augmented variant's ValidActions count.
    EXPECT_EQ(aug_target.pi.size(), aug_game.ValidActions().size());
    // Total mass must be preserved (1 in, 1 out modulo float rounding).
    float sum = 0.0F;
    for (float v : aug_target.pi) sum += v;
    EXPECT_TRUE(ApproxEqual(sum, 1.0F))
        << "Total probability mass must be preserved by symmetry; "
        << "got sum=" << sum;
  }
}

TEST(TrainingAugmenter, IdentityVariantPiUnchanged) {
  // The identity-augmented (game, target) pair must have the SAME pi
  // as the input — proving pi is at least preserved through identity.
  const XqGame game;
  const std::vector<XqA> orig_actions = game.ValidActions();
  if (orig_actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi(orig_actions.size(),
                        1.0F / static_cast<float>(orig_actions.size()));
  // Make pi[0] distinctive so it survives any erroneous permutation.
  if (!pi.empty()) {
    pi[0] = 0.5F;
    const float remaining = 0.5F;
    for (std::size_t i = 1; i < pi.size(); ++i) {
      pi[i] = remaining / static_cast<float>(pi.size() - 1);
    }
  }
  const TrainingTarget target{0.1F, pi};

  const XqTrainingAugmenter trainer;
  const auto pairs = trainer.Augment(game, target);
  ASSERT_FALSE(pairs.empty());
  // result[0] is the identity per the documented convention.
  const auto& [identity_game, identity_target] = pairs.front();
  ASSERT_EQ(identity_target.pi.size(), pi.size());
  for (std::size_t i = 0; i < pi.size(); ++i) {
    EXPECT_TRUE(ApproxEqual(identity_target.pi[i], pi[i]))
        << "Identity augmentation must keep pi[i] unchanged; pos=" << i;
  }
}

}  // namespace
}  // namespace az::game::xq
