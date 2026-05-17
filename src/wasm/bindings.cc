#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "alpha-zero-api/ring_buffer.h"
#include "include/xq/game.h"
#include "include/xq/inference.h"
#include "include/xq/serializer.h"

using namespace emscripten;
using namespace az::game::xq;

// Thin wrapper around XqGame for JavaScript.
class XqGameJs {
 public:
  XqGameJs() : game_() {}
  explicit XqGameJs(bool starting_player) : game_(starting_player) {}

  uint32_t currentRound() const { return game_.CurrentRound(); }
  bool currentPlayer() const { return game_.CurrentPlayer(); }

  val lastPlayer() const {
    auto lp = game_.LastPlayer();
    if (lp.has_value()) {
      return val(lp.value());
    }
    return val::null();
  }

  val lastAction() const {
    auto la = game_.LastAction();
    if (la.has_value()) {
      val obj = val::object();
      obj.set("from", la.value().from);
      obj.set("to", la.value().to);
      return obj;
    }
    return val::null();
  }

  bool isOver() const { return game_.IsOver(); }
  bool isInCheck() const { return game_.IsInCheck(); }
  float getScore(bool player) const { return game_.GetScore(player); }
  uint8_t currentPositionRepeatCount() const {
    return game_.CurrentPositionRepeatCount();
  }
  std::string boardReadableString() const {
    return game_.BoardReadableString();
  }

  val getBoard() const {
    const auto& board = game_.GetBoard();
    return val(typed_memory_view(board.size(), board.data()));
  }

  val canonicalBoard() const {
    canonical_board_cache_ = game_.CanonicalBoard();
    return val(typed_memory_view(canonical_board_cache_.size(),
                                 canonical_board_cache_.data()));
  }

  val validActions() const {
    std::array<XqA, XqGame::kMaxLegalActions> actions;
    size_t count = game_.ValidActionsInto(actions);

    val result = val::array();
    for (size_t i = 0; i < count; ++i) {
      val obj = val::object();
      obj.set("from", actions[i].from);
      obj.set("to", actions[i].to);
      result.call<void>("push", obj);
    }
    return result;
  }

  void applyValidActionByIndex(uint32_t idx) {
    std::array<XqA, XqGame::kMaxLegalActions> actions;
    size_t count = game_.ValidActionsInto(actions);
    if (idx < count) {
      game_.ApplyActionInPlace(actions[idx]);
    }
  }

  void undoLastAction() { game_.UndoLastAction(); }

  std::string actionToString(val a) const {
    XqA action{a["from"].as<uint8_t>(), a["to"].as<uint8_t>()};
    return game_.ActionToString(action);
  }

  val actionFromString(std::string s) const {
    auto result = game_.ActionFromString(s);
    val obj = val::object();
    if (result.has_value()) {
      obj.set("ok", true);
      val act = val::object();
      act.set("from", result.value().from);
      act.set("to", result.value().to);
      obj.set("action", act);
    } else {
      obj.set("ok", false);
      obj.set("errorCode", static_cast<uint8_t>(result.error()));
    }
    return obj;
  }

  const XqGame& get_game() const { return game_; }

 private:
  XqGame game_;
  mutable XqB canonical_board_cache_;
};

class XqSerializerJs {
 public:
  XqSerializerJs() = default;

  val serializeCurrentState(const XqGameJs& g) const {
    az::game::api::RingBuffer<XqGame, std::array<XqGame, 0>> history;
    auto state =
        serializer_.SerializeCurrentState(g.get_game(), history.View());

    val result = val::array();
    for (float f : state) {
      result.call<void>("push", f);
    }
    return result;
  }

  val serializePolicyOutput(const XqGameJs& g) const {
    // Produce a uniform target as a probe
    std::array<XqA, XqGame::kMaxLegalActions> valid_actions;
    size_t count = g.get_game().ValidActionsInto(valid_actions);

    az::game::api::TrainingTarget target;
    target.z = 0.0f;
    if (count > 0) {
      float uniform_prob = 1.0f / count;
      for (size_t i = 0; i < count; ++i) {
        target.pi.push_back(uniform_prob);
      }
    }

    auto output = serializer_.SerializePolicyOutput(g.get_game(), target);

    val result = val::array();
    for (float f : output) {
      result.call<void>("push", f);
    }
    return result;
  }

  val getAugmentationStats(const XqGameJs& g) const {
    az::game::xq::XqInferenceAugmenter augmenter;
    auto variants = augmenter.Augment(g.get_game());

    val result = val::object();
    result.set("variantCount", val(variants.size()));

    val variantActionCounts = val::array();
    for (const auto& variant : variants) {
      std::array<XqA, XqGame::kMaxLegalActions> actions;
      size_t count = variant.ValidActionsInto(actions);
      variantActionCounts.call<void>("push", val(count));
    }
    result.set("variantActionCounts", variantActionCounts);

    return result;
  }

 private:
  XqSerializer serializer_;
};

// Debug probe: produces a single snapshot of "everything the dev panel
// wants to see" from one `XqGame` — valid-action counts, the dense
// state vector + its self-roundtrip check, and the augmented variants
// each with their own board / valid-action count / roundtrip flag.
//
// The state vector's lossless reconstruction comparison is done in C++
// so the alpha-zero training engine can reuse the same primitive as a
// sanity check (the comparison is just "decode one-hot piece planes
// back to a board and equate to CanonicalBoard()").
//
// Boards are returned as `typed_memory_view`s into stable member
// storage; each call to `probe()` overwrites the previous snapshot, so
// JS callers must copy any data they want to keep before the next
// probe.
class XqDebugProbeJs {
 public:
  XqDebugProbeJs() = default;

  val probe(const XqGameJs& g) {
    const XqGame& game = g.get_game();

    // Cache: state vector, original canonical board, augmented variants.
    state_cache_ = serializer_.SerializeCurrentState(game, history_.View());
    canonical_cache_ = game.CanonicalBoard();
    variants_cache_ = augmenter_.Augment(game);

    val result = val::object();

    std::array<XqA, XqGame::kMaxLegalActions> actions;
    const size_t valid_count = game.ValidActionsInto(actions);
    result.set("validActionCount", val(static_cast<uint32_t>(valid_count)));

    result.set("stateVectorLength",
               val(static_cast<uint32_t>(state_cache_.size())));
    result.set(
        "stateRoundtripOk",
        val(DenseStateMatchesCanonical(state_cache_, canonical_cache_)));

    result.set("canonicalBoard",
               val(typed_memory_view(canonical_cache_.size(),
                                     canonical_cache_.data())));

    val variants_arr = val::array();
    for (size_t i = 0; i < variants_cache_.size(); ++i) {
      val v = val::object();
      v.set("index", val(static_cast<uint32_t>(i)));
      v.set("name", val(VariantName(i)));
      v.set("currentPlayer", val(variants_cache_[i].CurrentPlayer()));

      std::array<XqA, XqGame::kMaxLegalActions> v_actions;
      const size_t v_count = variants_cache_[i].ValidActionsInto(v_actions);
      v.set("validActionCount", val(static_cast<uint32_t>(v_count)));

      const XqB& vb = variants_cache_[i].GetBoard();
      v.set("board", val(typed_memory_view(vb.size(), vb.data())));

      const std::vector<float> v_state =
          serializer_.SerializeCurrentState(variants_cache_[i], history_.View());
      const XqB v_canonical = variants_cache_[i].CanonicalBoard();
      v.set("stateVectorLength",
            val(static_cast<uint32_t>(v_state.size())));
      v.set("stateRoundtripOk",
            val(DenseStateMatchesCanonical(v_state, v_canonical)));

      variants_arr.call<void>("push", v);
    }
    result.set("variants", variants_arr);

    return result;
  }

 private:
  XqSerializer serializer_;
  XqInferenceAugmenter augmenter_;
  // Empty history view: Xiang Qi is Markov so the serializer ignores it.
  az::game::api::RingBuffer<XqGame, std::array<XqGame, 0>> history_;
  std::vector<float> state_cache_;
  XqB canonical_cache_{};
  std::vector<XqGame> variants_cache_;

  static std::string VariantName(size_t i) {
    switch (i) {
      case 0:
        return "Original";
      case 1:
        return "Mirror";
      default:
        return "Variant " + std::to_string(i);
    }
  }

  // Decode the dense 15-plane state vector back into a canonical board
  // and compare against the engine-provided canonical board. The dense
  // serializer is a one-hot encoding so this is lossless on the board
  // dimension; a mismatch indicates a layout bug.
  static bool DenseStateMatchesCanonical(std::span<const float> state,
                                         const XqB& canonical) {
    if (state.size() != kDenseStateFeatureSize) return false;
    XqB reconstructed{};
    for (size_t cell = 0; cell < kBoardCells; ++cell) {
      for (size_t plane = 0; plane < 14; ++plane) {
        if (state[plane * kBoardCells + cell] >= 0.5F) {
          const int8_t code =
              plane < 7 ? static_cast<int8_t>(plane + 1)
                        : static_cast<int8_t>(-(static_cast<int>(plane) -
                                                7 + 1));
          reconstructed[cell] = code;
          break;
        }
      }
    }
    return reconstructed == canonical;
  }
};

EMSCRIPTEN_BINDINGS(xq_module) {
  class_<XqGameJs>("XqGameJs")
      .constructor<>()
      .constructor<bool>()
      .function("currentRound", &XqGameJs::currentRound)
      .function("currentPlayer", &XqGameJs::currentPlayer)
      .function("lastPlayer", &XqGameJs::lastPlayer)
      .function("lastAction", &XqGameJs::lastAction)
      .function("isOver", &XqGameJs::isOver)
      .function("isInCheck", &XqGameJs::isInCheck)
      .function("getScore", &XqGameJs::getScore)
      .function("currentPositionRepeatCount",
                &XqGameJs::currentPositionRepeatCount)
      .function("boardReadableString", &XqGameJs::boardReadableString)
      .function("getBoard", &XqGameJs::getBoard)
      .function("canonicalBoard", &XqGameJs::canonicalBoard)
      .function("validActions", &XqGameJs::validActions)
      .function("applyValidActionByIndex", &XqGameJs::applyValidActionByIndex)
      .function("undoLastAction", &XqGameJs::undoLastAction)
      .function("actionToString", &XqGameJs::actionToString)
      .function("actionFromString", &XqGameJs::actionFromString);

  class_<XqSerializerJs>("XqSerializerJs")
      .constructor<>()
      .function("serializeCurrentState", &XqSerializerJs::serializeCurrentState)
      .function("serializePolicyOutput", &XqSerializerJs::serializePolicyOutput)
      .function("getAugmentationStats", &XqSerializerJs::getAugmentationStats);

  class_<XqDebugProbeJs>("XqDebugProbeJs")
      .constructor<>()
      .function("probe", &XqDebugProbeJs::probe);
}
