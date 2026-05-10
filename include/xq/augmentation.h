#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_AUGMENTATION_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_AUGMENTATION_H_

#include <cstdint>
#include <vector>

#include "include/xq/game.h"

namespace az::game::xq::internal {

/**
 * @brief Identifiers for each augmented variant of a Xiang Qi game
 * state.
 *
 * Xiang Qi has a single useful board symmetry: a left/right mirror
 * across the central file (column 4). The river divides the two
 * sides asymmetrically and the palaces are tied to specific files,
 * so rotations and the vertical flip are NOT symmetries of the
 * game.
 *
 * Conventions:
 *   - `kOriginal = 0` is the identity. `AugmentAll(game)[0]`
 *     reproduces `game`'s observable state.
 *   - `kMirrorHorizontal = 1` is the column mirror; cell `(r, c)`
 *     maps to `(r, kBoardCols - 1 - c)`.
 *   - The enum order matches `internal::AugmentAll(game)` output.
 *   - Underlying type `uint8_t` so the index can be used as a
 *     hash-map key or an inverse-action lookup index.
 */
enum class XqAugmentation : uint8_t {
  kOriginal = 0,
  kMirrorHorizontal = 1,
};

/**
 * @brief Number of supported augmentation variants. Equal to the
 * cardinality of `XqAugmentation` and the size of every
 * `AugmentAll(game)` return value.
 */
inline constexpr std::size_t kNumAugmentations = 2;

/**
 * @brief Mirror an action across the central file.
 *
 * Both `from` and `to` cells have their column reflected:
 * `c -> kBoardCols - 1 - c`. Self-inverse.
 */
[[nodiscard]] XqA MirrorHorizontal(const XqA& action) noexcept;

/**
 * @brief Mirror a game state across the central file.
 *
 * Returns a snapshot game with the board, last action (if any),
 * and side-to-move all reflected. The current player is unchanged
 * because horizontal mirror does not swap colors.
 */
[[nodiscard]] XqGame MirrorHorizontal(const XqGame& game) noexcept;

/**
 * @brief Apply every supported augmentation to `game` and return
 * the resulting variants in `XqAugmentation` enum order.
 *
 * `result.size() == kNumAugmentations`. `result[0]` is the
 * identity (the input itself); `result[1]` is the horizontal
 * mirror. The `XqGame` instances returned satisfy the same `Game`
 * concept as the input — the legal-action count returned by `ValidActionsInto` is preserved
 * because mirroring is a bijection on legal moves.
 */
[[nodiscard]] std::vector<XqGame> AugmentAll(const XqGame& game) noexcept;

/**
 * @brief Map an action expressed in an augmented coordinate frame
 * back to the original frame. Self-inverse for every supported
 * symmetry (identity is trivially self-inverse; mirror is its own
 * inverse).
 */
[[nodiscard]] XqA InverseTransformAction(const XqA& augmented_action,
                                         XqAugmentation augmentation) noexcept;

}  // namespace az::game::xq::internal

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_AUGMENTATION_H_
