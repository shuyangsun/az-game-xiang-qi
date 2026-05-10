#include "include/xq/augmentation.h"

#include <vector>

#include "include/xq/game.h"

namespace az::game::xq::internal {

std::vector<XqGame> AugmentAll(const XqGame& game) noexcept {
  // TODO(TASK-AUGMENTATION-IMPL): return one variant per member of
  // XqAugmentation, in enum order. result[0] must
  // be the identity. Add helpers (rotate, mirror, etc.) above as needed;
  // declare them in the header if other modules need to invert them.

  std::vector<XqGame> result;
  result.reserve(1);
  result.push_back(game);
  return result;
}

}  // namespace az::game::xq::internal
