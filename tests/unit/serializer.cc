#include "gtest/gtest.h"

namespace az::game::xq {
namespace {

TEST(Serializer, SerializeCurrentStateHasFixedLength) {
  GTEST_SKIP() << "TODO(TASK-SERIALIZER-TEST): verify SerializeCurrentState "
                  "returns the documented fixed length across reachable game "
                  "states.";
}

TEST(Serializer, SerializeCurrentStateEncodesPlayerToMove) {
  GTEST_SKIP() << "TODO(TASK-SERIALIZER-TEST): verify the encoding reflects "
                  "whose turn it is.";
}

TEST(Serializer, SerializePolicyOutputHasFixedLength) {
  GTEST_SKIP() << "TODO(TASK-SERIALIZER-TEST): verify SerializePolicyOutput "
                  "returns the documented fixed length.";
}

TEST(Serializer, SerializePolicyOutputPlacesValueFirst) {
  GTEST_SKIP() << "TODO(TASK-SERIALIZER-TEST): verify result[0] equals "
                  "TrainingTarget.z.";
}

TEST(Serializer, SerializePolicyOutputZeroesInvalidActions) {
  GTEST_SKIP() << "TODO(TASK-SERIALIZER-TEST): verify slots for actions whose "
                  "PolicyIndex is outside the current ValidActions() set have "
                  "zero probability.";
}

}  // namespace
}  // namespace az::game::xq
