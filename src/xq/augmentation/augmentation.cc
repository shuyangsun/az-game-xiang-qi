#include "include/xq/augmentation.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "include/xq/game.h"
#include "src/xq/game/internal.h"

namespace az::game::xq::internal {

namespace {

// Mirror a single cell index across the central file (col 4):
// `(r, c) -> (r, kBoardCols - 1 - c)`.
constexpr uint8_t MirrorCell(uint8_t cell) noexcept {
  const uint8_t r = Row(cell);
  const uint8_t c = Col(cell);
  return Idx(r, static_cast<uint8_t>(kBoardCols - 1 - c));
}

}  // namespace

XqA MirrorHorizontal(const XqA& action) noexcept {
  return XqA{MirrorCell(action.from), MirrorCell(action.to)};
}

XqGame MirrorHorizontal(const XqGame& game) noexcept {
  // Build the mirrored board by reflecting every cell. Piece codes
  // are unchanged because horizontal mirror does not swap colors.
  XqB board{};
  const XqB& src = game.GetBoard();
  for (uint8_t cell = 0; cell < kBoardCells; ++cell) {
    board[MirrorCell(cell)] = src[cell];
  }

  // Mirror the most recent action so `LastAction()` remains
  // meaningful on the augmented snapshot. Older history is dropped
  // because augmenters operate on single positions, not on
  // playthroughs (see memory/game_design_details/repetition.md).
  std::optional<XqA> last = game.LastAction();
  if (last.has_value()) {
    last = MirrorHorizontal(*last);
  }

  return XqGame(board, game.CurrentPlayer(), game.CurrentRound(), last);
}

std::vector<XqGame> AugmentAll(const XqGame& game) noexcept {
  std::vector<XqGame> result;
  result.reserve(kNumAugmentations);
  result.push_back(game);                   // kOriginal
  result.push_back(MirrorHorizontal(game));  // kMirrorHorizontal
  return result;
}

XqA InverseTransformAction(const XqA& augmented_action,
                           XqAugmentation augmentation) noexcept {
  // Both supported transforms are self-inverse: the identity is
  // trivially so, and a horizontal mirror applied twice returns the
  // original action.
  switch (augmentation) {
    case XqAugmentation::kOriginal:
      return augmented_action;
    case XqAugmentation::kMirrorHorizontal:
      return MirrorHorizontal(augmented_action);
  }
  return augmented_action;
}

}  // namespace az::game::xq::internal
