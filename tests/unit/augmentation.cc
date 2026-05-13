#include "include/xq/augmentation.h"

#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "include/xq/game.h"
#include "tests/unit/valid_actions.h"

namespace az::game::xq::internal {
namespace {

constexpr uint8_t Idx(uint8_t row, uint8_t col) noexcept {
  return static_cast<uint8_t>(row * kBoardCols + col);
}

TEST(Augmentation, FR_AUG_CARDINALITY_AugmentAllSizeMatchesEnum) {
  const XqGame game;
  EXPECT_EQ(AugmentAll(game).size(), kNumAugmentations);
}

TEST(Augmentation, FR_AUG_IDENTITY_FIRST_FirstEntryIsOriginal) {
  const XqGame game;
  const std::vector<XqGame> aug = AugmentAll(game);
  ASSERT_FALSE(aug.empty());
  EXPECT_EQ(aug.front().GetBoard(), game.GetBoard());
  EXPECT_EQ(aug.front().CurrentPlayer(), game.CurrentPlayer());
}

TEST(Augmentation, FR_AUG_ORDER_IndicesMatchEnumValues) {
  const XqGame game;
  const std::vector<XqGame> aug = AugmentAll(game);
  ASSERT_EQ(aug.size(), kNumAugmentations);
  // Original at index 0 (kOriginal == 0).
  EXPECT_EQ(aug[static_cast<size_t>(XqAugmentation::kOriginal)].GetBoard(),
            game.GetBoard());
  // Mirror at index 1 (kMirrorHorizontal == 1).
  const XqGame& mirror =
      aug[static_cast<size_t>(XqAugmentation::kMirrorHorizontal)];
  // The mirror must equal MirrorHorizontal(game).
  const XqGame expected = MirrorHorizontal(game);
  EXPECT_EQ(mirror.GetBoard(), expected.GetBoard());
}

TEST(Augmentation, FR_AUG_PRESERVES_ACTION_COUNT_MirrorPreservesValidActions) {
  const XqGame game;
  const std::vector<XqGame> aug = AugmentAll(game);
  ASSERT_EQ(aug.size(), kNumAugmentations);
  const size_t expected = ValidActions(game).size();
  for (const XqGame& v : aug) {
    EXPECT_EQ(ValidActions(v).size(), expected);
  }
}

TEST(Augmentation, FR_AUG_MIRROR_INVOLUTION_TwoMirrorsRestoreBoard) {
  const XqGame game;
  const XqGame once = MirrorHorizontal(game);
  const XqGame twice = MirrorHorizontal(once);
  EXPECT_EQ(twice.GetBoard(), game.GetBoard());
}

TEST(Augmentation, MirrorHorizontalActionReflectsColumns) {
  // Build a known move (b0 -> c2 = horse opening). Mirror should map
  // it to (h0 -> g2): col 1 -> 7 and col 2 -> 6.
  const XqA original{Idx(0, 1), Idx(2, 2)};
  const XqA mirrored = MirrorHorizontal(original);
  EXPECT_EQ(mirrored.from, Idx(0, 7));
  EXPECT_EQ(mirrored.to, Idx(2, 6));
}

TEST(Augmentation, FR_AUG_INVERSE_ACTION_MirrorIsSelfInverse) {
  const XqA original{Idx(3, 4), Idx(5, 4)};
  const XqA mirrored = MirrorHorizontal(original);
  const XqA back =
      InverseTransformAction(mirrored, XqAugmentation::kMirrorHorizontal);
  EXPECT_EQ(back.from, original.from);
  EXPECT_EQ(back.to, original.to);
}

TEST(Augmentation, FR_AUG_INVERSE_ACTION_IdentityIsIdentity) {
  const XqA original{Idx(2, 4), Idx(2, 5)};
  const XqA back = InverseTransformAction(original, XqAugmentation::kOriginal);
  EXPECT_EQ(back.from, original.from);
  EXPECT_EQ(back.to, original.to);
}

TEST(Augmentation, FR_AUG_VARIANT_IS_GAME_AllVariantsRoundtripValidActions) {
  const XqGame game;
  const std::vector<XqGame> aug = AugmentAll(game);
  for (const XqGame& v : aug) {
    // Each variant must be queryable as a Game (no crash, valid output).
    const std::vector<XqA> actions = ValidActions(v);
    EXPECT_EQ(actions.size(), ValidActions(v).size())
        << "Variant ValidActions() must be deterministic.";
  }
}

TEST(Augmentation, MirrorMatchesCellLevel) {
  // Verify the geometric mirror cell-by-cell.
  const XqGame game;
  const XqGame mirror = MirrorHorizontal(game);
  for (uint8_t r = 0; r < kBoardRows; ++r) {
    for (uint8_t c = 0; c < kBoardCols; ++c) {
      const uint8_t mirror_c = static_cast<uint8_t>(kBoardCols - 1 - c);
      EXPECT_EQ(mirror.GetBoard()[Idx(r, mirror_c)], game.GetBoard()[Idx(r, c)])
          << "Mirror failure at (" << static_cast<int>(r) << ", "
          << static_cast<int>(c) << ").";
    }
  }
}

}  // namespace
}  // namespace az::game::xq::internal
