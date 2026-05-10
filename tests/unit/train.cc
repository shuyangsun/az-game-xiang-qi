#include "gtest/gtest.h"

namespace az::game::xq {
namespace {

TEST(TrainingAugmenter, AugmentReturnsOnePerVariant) {
  GTEST_SKIP() << "TODO(TASK-TRAIN-TEST): verify Augment() returns one pair "
                  "per supported augmentation.";
}

TEST(TrainingAugmenter, AugmentPermutesPolicyAlongsideBoard) {
  GTEST_SKIP() << "TODO(TASK-TRAIN-TEST): verify TrainingTarget.pi is "
                  "permuted to match each augmented game's ValidActions().";
}

TEST(TrainingAugmenter, AugmentPreservesValue) {
  GTEST_SKIP() << "TODO(TASK-TRAIN-TEST): verify TrainingTarget.z is the "
                  "same in every augmented training pair.";
}

}  // namespace
}  // namespace az::game::xq
