#include "gtest/gtest.h"

namespace az::game::xq {
namespace {

// TODO(TASK-GAME-ACTION-TEST): add tests for `ValidActions`, `PolicyIndex`,
// `ApplyActionInPlace`, and `UndoLastAction`. Make sure test names are
// meaningful but not too long.

TEST(GameAction, ValidActions1) {
  GTEST_SKIP() << "TODO(TASK-GAME-ACTION-TEST): verify ValidActions() for the "
                  "initial state.";
}

TEST(GameAction, ApplyActionInPlace1) {
  GTEST_SKIP() << "TODO(TASK-GAME-ACTION-TEST): verify ApplyActionInPlace() "
                  "for a valid action.";
}

TEST(GameAction, UndoLastActionRestoresState) {
  GTEST_SKIP() << "TODO(TASK-GAME-ACTION-TEST): verify UndoLastAction() "
                  "restores the previous state.";
}

TEST(GameAction, PolicyIndexIsBijection) {
  GTEST_SKIP() << "TODO(TASK-GAME-ACTION-TEST): verify PolicyIndex() is a "
                  "bijection over the action space.";
}

}  // namespace
}  // namespace az::game::xq
