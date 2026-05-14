#ifndef ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_SERIALIZER_H_
#define ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_SERIALIZER_H_

#include <cstddef>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "alpha-zero-api/ring_buffer.h"
#include "alpha-zero-api/serializer.h"
#include "include/xq/game.h"

namespace az::game::xq {

inline constexpr size_t kStateFeaturePlanes = 15;
inline constexpr size_t kDenseStateFeatureSize =
    kStateFeaturePlanes * kBoardCells;

// Compact (transformer-oriented) state layout. The state vector is a
// flat concatenation of `kCompactStateTokenCount` uniform tokens, each
// `kCompactTokenFeatureWidth = 2` floats wide:
//
//   tokens 0 .. kBoardCells - 1               : board cells,
//                                                 [piece_code, 0]
//   token  kBoardCells                        : repeat counter,
//                                                 [repeat_count, 0]
//   tokens kBoardCells + 1 .. (end - 1)       : legal-action slots,
//                                                 [from, to]
//
// Total length is `kCompactStateFeatureSize = kCompactStateTokenCount *
// kCompactTokenFeatureWidth = 195 * 2 = 390`. Action slots are sorted
// by canonical `(from, to)` ascending; padding slots reuse the
// `XqA{kBoardCells, kBoardCells}` "no action" sentinel for both
// features. Board and repeat tokens leave feature slot 1 as a zero pad
// so a single transformer per-token embedding can project the entire
// sequence with one `W_embed`.
inline constexpr size_t kCompactTokenFeatureWidth = 2;
inline constexpr size_t kCompactStateTokenCount =
    kBoardCells + 1 + XqGame::kMaxLegalActions;
inline constexpr size_t kCompactStateFeatureSize =
    kCompactStateTokenCount * kCompactTokenFeatureWidth;

// Token-index offsets within the state vector. Multiply by
// `kCompactTokenFeatureWidth` to get the float-index offset.
inline constexpr size_t kCompactBoardTokenOffset = 0;
inline constexpr size_t kCompactRepeatTokenOffset = kBoardCells;
inline constexpr size_t kCompactActionTokenOffset = kBoardCells + 1;

/**
 * @brief Serializes Xiang Qi game state and training
 * target to fixed-size float vectors suitable for neural network
 * input/output.
 *
 * TODO(TASK-SERIALIZER-IMPL): tailor this docstring to be Xiang Qi
 * specific. Describe the input encoding (channels, shape, normalization)
 * and the output encoding (action layout via `PolicyIndex`, value-slot
 * placement, padding strategy for invalid actions).
 *
 * Implements both `::az::game::api::IGameSerializer<XqGame>`
 * and `::az::game::api::IPolicyOutputSerializer<XqGame>`
 * to keep the encoding logic for the neural network input and the training
 * labels together.
 */
class XqSerializer : public ::az::game::api::IGameSerializer<XqGame>,
                     public ::az::game::api::IPolicyOutputSerializer<XqGame> {
 public:
  XqSerializer() = default;
  ~XqSerializer() override = default;

  /**
   * @brief Serialize the current game state into a fixed-size float
   * vector that is passed as input to the policy/value neural network.
   *
   * TODO(TASK-SERIALIZER-IMPL): tailor this docstring once the input
   * encoding is decided. Common encodings include one-hot per piece type,
   * channels for "to-move" / castling rights / move counters, etc.
   * For non-Markov games, `history` provides the most-recent
   * `kHistoryLookback` past states (index 0 = newest); use them if the
   * input encoding requires temporal context.
   *
   * The returned vector MUST be of fixed length across all reachable game
   * states because it is consumed by the neural network as a tensor.
   *
   * @param game Current game state.
   * @param history Engine-owned window over recent past states; size is
   * bounded by `XqGame::kHistoryLookback`. `game`
   * itself is not in the view.
   * @return std::vector<float> Neural network input tensor (flattened).
   */
  [[nodiscard]] std::vector<float> SerializeCurrentState(
      const XqGame& game,
      ::az::game::api::RingBufferView<XqGame> history) const noexcept final;

  /**
   * @brief Serialize a `TrainingTarget` (z, π) into a fixed-size float
   * vector suitable for use as a network training target.
   *
   * TODO(TASK-SERIALIZER-IMPL): tailor this docstring once the output
   * encoding is decided. The canonical layout puts `target.z` in slot 0
   * and the `kPolicySize`-wide policy distribution starting at slot 1,
   * scattered via `game.PolicyIndex(action)`.
   *
   * The returned vector MUST be of fixed length across all reachable game
   * states. Probability mass for actions not in `game.ValidActionsInto(...)`
   * should be zero so the network learns the valid-action mask
   * implicitly.
   *
   * `target.pi[i]` corresponds to the i-th entry written by
   * `game.ValidActionsInto(buf)` into the caller-owned buffer. The
   * implementation is responsible for scattering those values into the
   * fixed-size policy slot via `game.PolicyIndex(action)`.
   *
   * @param game Game state the target was produced for.
   * @param target Training target (`z`, `pi`).
   * @return std::vector<float> Neural network output tensor (flattened).
   */
  [[nodiscard]] std::vector<float> SerializePolicyOutput(
      const XqGame& game,
      const ::az::game::api::TrainingTarget& target) const noexcept final;
};

/**
 * @brief Compact Xiang Qi serializer for transformer-style networks
 * whose policy head is sized to `XqGame::kMaxLegalActions` instead of
 * `XqGame::kPolicySize`.
 *
 * The input vector is a flat concatenation of
 * `kCompactStateTokenCount = 195` uniform tokens, each
 * `kCompactTokenFeatureWidth = 2` floats wide, for a total length of
 * `kCompactStateFeatureSize = 390`:
 *
 *   1. Board tokens (tokens 0..89). One token per cell of
 *      `CanonicalBoard()`, row-major. Token feature 0 holds the signed
 *      piece code in `[-7, +7]` (positive = current player); feature 1
 *      is a zero pad. No one-hot expansion — the consuming network is
 *      expected to learn its own per-piece embedding from the scalar
 *      piece code.
 *   2. Repeat-counter token (token 90). Feature 0 holds
 *      `XqGame::CurrentPositionRepeatCount()` (1, 2, or 3) so the
 *      network can see proximity to the threefold-repetition draw;
 *      feature 1 is a zero pad. Its distinct sequence position lets
 *      the transformer's position embedding distinguish it from a
 *      board cell with the same scalar value.
 *   3. Legal-action tokens (tokens 91..194). Each token is
 *      `(from, to)` in canonical cell coordinates in
 *      `[0, kBoardCells)`. Real tokens are sorted by canonical
 *      `(from, to)` ascending. Padding tokens reuse the existing
 *      `XqA{kBoardCells, kBoardCells}` "no action" sentinel for both
 *      features.
 *
 * The legal-action token ordering is part of the public contract: the
 * i-th legal-action token in this input vector matches the i-th
 * non-padding entry of `SerializePolicyOutput`'s
 * `CompactPolicyTargetBlob`, so callers can train and infer in the
 * same frame without an explicit index map.
 *
 * `SerializePolicyOutput` returns the same sorted compact row as a
 * `CompactPolicyTargetBlob`: `count` is the unpadded legal-action count,
 * vectors are padded to `XqGame::kMaxLegalActions`, and padding indices use
 * `CompactPolicyTargetBlob::kPaddingSlot`.
 */
class XqCompactSerializer
    : public ::az::game::api::IGameSerializer<XqGame>,
      public ::az::game::api::ICompactPolicyOutputSerializer<XqGame> {
 public:
  XqCompactSerializer() = default;
  ~XqCompactSerializer() override = default;

  [[nodiscard]] std::vector<float> SerializeCurrentState(
      const XqGame& game,
      ::az::game::api::RingBufferView<XqGame> history) const noexcept final;

  [[nodiscard]] ::az::game::api::CompactPolicyTargetBlob SerializePolicyOutput(
      const XqGame& game,
      const ::az::game::api::TrainingTarget& target) const noexcept final;
};

}  // namespace az::game::xq

#endif  // ALPHA_ZERO_GAME_XIANG_QI_INCLUDE_XQ_SERIALIZER_H_
