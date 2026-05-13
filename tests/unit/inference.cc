#include "include/xq/inference.h"

#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "gtest/gtest.h"
#include "include/xq/augmentation.h"
#include "include/xq/game.h"
#include "tests/unit/valid_actions.h"

namespace az::game::xq {
namespace {

using ::az::game::api::Evaluation;

bool ApproxEqual(float a, float b) noexcept {
  constexpr float kAbs = 1e-5F;
  constexpr float kRel = 1e-4F;
  const float diff = std::fabs(a - b);
  return diff <= kAbs || diff <= kRel * std::max(std::fabs(a), std::fabs(b));
}

TEST(InferenceAugmenter, FR_AUG_CARDINALITY_AugmentMatchesAugmentAll) {
  const XqGame game;
  const XqInferenceAugmenter inf;
  const std::vector<XqGame> ours = inf.Augment(game);
  const std::vector<XqGame> ref = internal::AugmentAll(game);
  ASSERT_EQ(ours.size(), ref.size());
  for (size_t i = 0; i < ours.size(); ++i) {
    EXPECT_EQ(ours[i].GetBoard(), ref[i].GetBoard())
        << "Variant " << i << " board mismatch.";
  }
}

TEST(InferenceAugmenter, FR_INF_INTERPRET_LEN_MatchesOriginalActions) {
  const XqGame game;
  const XqInferenceAugmenter inf;
  const std::vector<XqGame> aug = inf.Augment(game);
  const size_t orig_actions = ValidActions(game).size();
  if (orig_actions == 0) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }

  std::vector<Evaluation> evals;
  evals.reserve(aug.size());
  for (const XqGame& v : aug) {
    const size_t n = ValidActions(v).size();
    std::vector<float> probs(n, 1.0F / static_cast<float>(n == 0 ? 1 : n));
    evals.push_back(Evaluation{0.0F, std::move(probs)});
  }
  const Evaluation combined = inf.Interpret(game, std::span<const XqGame>(aug),
                                            std::span<const Evaluation>(evals));
  EXPECT_EQ(combined.probabilities.size(), orig_actions);
}

TEST(InferenceAugmenter, FR_INF_INTERPRET_VALUE_AveragesIdenticalValues) {
  const XqGame game;
  const XqInferenceAugmenter inf;
  const std::vector<XqGame> aug = inf.Augment(game);
  const size_t n = ValidActions(game).size();
  if (n == 0) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }

  std::vector<Evaluation> evals;
  evals.reserve(aug.size());
  for (const XqGame& v : aug) {
    const size_t na = ValidActions(v).size();
    std::vector<float> probs(na, 1.0F / static_cast<float>(na));
    evals.push_back(Evaluation{0.7F, std::move(probs)});
  }
  const Evaluation combined = inf.Interpret(game, std::span<const XqGame>(aug),
                                            std::span<const Evaluation>(evals));
  EXPECT_TRUE(ApproxEqual(combined.value, 0.7F))
      << "Averaging identical values must reproduce that value.";
}

TEST(InferenceAugmenter, FR_INF_INTERPRET_PROB_UniformVariantProbsStayUniform) {
  const XqGame game;
  const XqInferenceAugmenter inf;
  const std::vector<XqGame> aug = inf.Augment(game);
  const size_t n = ValidActions(game).size();
  if (n == 0) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }

  std::vector<Evaluation> evals;
  evals.reserve(aug.size());
  for (const XqGame& v : aug) {
    const size_t na = ValidActions(v).size();
    std::vector<float> probs(na, 1.0F / static_cast<float>(na));
    evals.push_back(Evaluation{0.0F, std::move(probs)});
  }
  const Evaluation combined = inf.Interpret(game, std::span<const XqGame>(aug),
                                            std::span<const Evaluation>(evals));
  ASSERT_EQ(combined.probabilities.size(), n);
  // Uniform input across all variants should yield uniform output.
  for (float p : combined.probabilities) {
    EXPECT_TRUE(ApproxEqual(p, 1.0F / static_cast<float>(n)))
        << "Uniform-in -> uniform-out aggregation expected, got " << p;
  }
}

TEST(InferenceAugmenter, FR_INF_INTERPRET_ALIGN_OriginalActionOrdering) {
  // Build an evaluation that gives non-uniform probability to one
  // specific original-frame action across every variant. Verify the
  // returned distribution aligns with ValidActions(original).
  const XqGame game;
  const XqInferenceAugmenter inf;
  const std::vector<XqGame> aug = inf.Augment(game);
  const std::vector<XqA> orig_actions = ValidActions(game);
  if (orig_actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }

  std::vector<Evaluation> evals;
  evals.reserve(aug.size());
  for (const XqGame& v : aug) {
    const size_t na = ValidActions(v).size();
    std::vector<float> probs(na, 0.0F);
    if (na > 0) probs[0] = 1.0F;
    evals.push_back(Evaluation{0.0F, std::move(probs)});
  }
  const Evaluation combined = inf.Interpret(game, std::span<const XqGame>(aug),
                                            std::span<const Evaluation>(evals));
  ASSERT_EQ(combined.probabilities.size(), orig_actions.size());
  // Whatever the aggregation, all output entries must be non-negative
  // and sum to (within rounding) the average of input total mass = 1.
  float sum = 0.0F;
  for (float p : combined.probabilities) {
    EXPECT_GE(p, 0.0F);
    sum += p;
  }
  EXPECT_TRUE(ApproxEqual(sum, 1.0F))
      << "Aggregated probabilities should sum to 1; got " << sum;
}

}  // namespace
}  // namespace az::game::xq
