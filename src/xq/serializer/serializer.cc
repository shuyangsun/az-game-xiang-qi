#include "include/xq/serializer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "alpha-zero-api/ring_buffer.h"
#include "include/xq/game.h"
#include "src/xq/game/internal.h"

namespace az::game::xq {

namespace {

using ::az::game::xq::internal::IsRed;

// 14 piece-type planes (7 own + 7 opponent) + 1 side-to-move plane.
constexpr size_t kNumPlanes = 15;

constexpr size_t PlaneOffset(size_t plane) noexcept {
  return plane * kBoardCells;
}

}  // namespace

std::vector<float> XqSerializer::SerializeCurrentState(
    const XqGame& game,
    ::az::game::api::RingBufferView<XqGame> /*history*/) const noexcept {
  // Markov game: history view is unused. Layout is plane-major:
  //   planes  0..6: own-piece planes (General .. Soldier)
  //   planes  7..13: opponent-piece planes (same order)
  //   plane   14:    side-to-move plane (all 1s if Red to move, else 0)
  // Each plane holds 90 cells in row-major (`r * 9 + c`) order.
  std::vector<float> out(kNumPlanes * kBoardCells, 0.0F);

  // Use the canonical board so own pieces are positive (codes 1..7)
  // and opponent pieces are negative (codes -1..-7).
  const XqB canonical = game.CanonicalBoard();
  for (size_t cell = 0; cell < kBoardCells; ++cell) {
    const int8_t code = canonical[cell];
    if (code == 0) continue;
    const size_t plane = IsRed(code) ? static_cast<size_t>(code - 1)
                                     : static_cast<size_t>(7 + (-code - 1));
    out[PlaneOffset(plane) + cell] = 1.0F;
  }

  if (game.CurrentPlayer() == kRed) {
    const size_t base = PlaneOffset(14);
    std::fill(out.begin() + base, out.begin() + base + kBoardCells, 1.0F);
  }

  return out;
}

std::vector<float> XqSerializer::SerializePolicyOutput(
    const XqGame& game,
    const ::az::game::api::TrainingTarget& target) const noexcept {
  // Canonical layout shared with `XqDeserializer`:
  //   out[0]                     = target.z
  //   out[1 + PolicyIndex(a_i)]  = target.pi[i]   for actions[i] = a_i
  //   other slots                = 0
  std::vector<float> out(XqGame::kPolicySize + 1, 0.0F);
  out[0] = target.z;

  std::array<XqA, XqGame::kMaxLegalActions> actions{};
  const size_t count = game.ValidActionsInto(actions);
  const size_t n = std::min(count, target.pi.size());
  for (size_t i = 0; i < n; ++i) {
    const size_t slot = 1 + game.PolicyIndex(actions[i]);
    if (slot < out.size()) {
      out[slot] = target.pi[i];
    }
  }
  return out;
}

}  // namespace az::game::xq
