#include "gtest/gtest.h"

namespace az::game::xq::internal {
namespace {

TEST(Augmentation, AugmentAllIncludesOriginal) {
  GTEST_SKIP() << "TODO(TASK-AUGMENTATION-TEST): verify AugmentAll() always "
                  "contains an entry keyed by kOriginal that is identical to "
                  "the input.";
}

TEST(Augmentation, AugmentAllPreservesActionCount) {
  GTEST_SKIP() << "TODO(TASK-AUGMENTATION-TEST): verify every augmented "
                  "variant has the same number of valid actions as the "
                  "input.";
}

TEST(Augmentation, AugmentAllProducesUniqueKeys) {
  GTEST_SKIP() << "TODO(TASK-AUGMENTATION-TEST): verify each augmentation "
                  "key appears at most once.";
}

}  // namespace
}  // namespace az::game::xq::internal
