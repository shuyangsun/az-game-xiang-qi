#include "gtest/gtest.h"

namespace az::game::xq {
namespace {

TEST(Deserializer, RejectsWrongSize) {
  GTEST_SKIP() << "TODO(TASK-DESERIALIZER-TEST): verify Deserialize() returns "
                  "an error when the network output has the wrong length.";
}

TEST(Deserializer, ProbabilitiesParallelToValidActions) {
  GTEST_SKIP() << "TODO(TASK-DESERIALIZER-TEST): verify the returned "
                  "Evaluation.probabilities vector has the same length as "
                  "game.ValidActions() and corresponds 1:1.";
}

TEST(Deserializer, RoundTripsThroughSerializer) {
  GTEST_SKIP() << "TODO(TASK-DESERIALIZER-TEST): verify Deserialize composed "
                  "with Serializer::SerializePolicyOutput recovers the "
                  "original value/probabilities (up to renormalization).";
}

}  // namespace
}  // namespace az::game::xq
