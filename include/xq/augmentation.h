#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_AUGMENTATION_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_AUGMENTATION_H_

#include <cstdint>
#include <vector>

#include "include/xq/game.h"

namespace az::game::xq::internal {

/**
 * @brief Identifiers for each augmented variant of a Xiang Qi
 * game state.
 *
 * TODO(TASK-AUGMENTATION-ENUM): enumerate every augmentation that
 * preserves Xiang Qi game equivalence (e.g., rotations,
 * mirrors, color swaps). For games with no useful symmetry, keep just
 * `kOriginal` and the augmenters will degenerate into identity
 * transforms. See `memory/augmentation_strategy.md` for design
 * conventions (identity-first, ordering, common symmetry groups).
 */
enum class XqAugmentation : uint8_t {
  kOriginal = 0,
};

/**
 * @brief Apply every supported augmentation to `game` and return the
 * resulting variants.
 *
 * TODO(TASK-AUGMENTATION-IMPL): implement the augmentation logic. The
 * returned vector must have one entry per member of
 * `XqAugmentation`, in enum order, so the index of
 * each variant doubles as its augmentation key. `result[0]` must be the
 * identity (`game` itself).
 *
 * Each returned `XqGame` must satisfy the same `Game`
 * contract as the input — including `ValidActions()` size and
 * deterministic ordering — so per-variant network outputs can be
 * inverse-mapped back to the original action space.
 *
 * @param game Original game state.
 * @return Augmented variants in enum order.
 */
[[nodiscard]] std::vector<XqGame> AugmentAll(const XqGame& game) noexcept;

}  // namespace az::game::xq::internal

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_AUGMENTATION_H_
