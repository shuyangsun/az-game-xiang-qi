#include "gtest/gtest.h"

namespace az::game::xq {
namespace {

TEST(InferenceAugmenter, AugmentMatchesAugmentAll) {
  GTEST_SKIP() << "TODO(TASK-INFERENCE-TEST): verify XqInferenceAugmenter::"
                  "Augment matches the variants produced by "
                  "internal::AugmentAll for the inference set.";
}

TEST(InferenceAugmenter, InterpretReturnsOriginalActionSpace) {
  GTEST_SKIP() << "TODO(TASK-INFERENCE-TEST): verify Interpret() returns an "
                  "Evaluation whose probabilities are parallel to the "
                  "ORIGINAL valid actions.";
}

TEST(InferenceAugmenter, InterpretAveragesValuesAcrossVariants) {
  GTEST_SKIP() << "TODO(TASK-INFERENCE-TEST): feed Interpret() identical "
                  "values across variants and verify the result equals that "
                  "value.";
}

TEST(InferenceAugmenter, InterpretAggregatesProbabilities) {
  GTEST_SKIP() << "TODO(TASK-INFERENCE-TEST): feed Interpret() variant-frame "
                  "probabilities that all map to the same original action and "
                  "verify the aggregated mass matches expectations.";
}

}  // namespace
}  // namespace az::game::xq
