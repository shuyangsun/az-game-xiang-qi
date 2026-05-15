#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "include/xq/game.h"
#include "include/xq/serializer.h"
#include "include/xq/inference.h"

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
  uint8_t currentPositionRepeatCount() const { return game_.CurrentPositionRepeatCount(); }
  std::string boardReadableString() const { return game_.BoardReadableString(); }

  val getBoard() const {
    const auto& board = game_.GetBoard();
    return val(typed_memory_view(board.size(), board.data()));
  }

  val canonicalBoard() const {
    canonical_board_cache_ = game_.CanonicalBoard();
    return val(typed_memory_view(canonical_board_cache_.size(), canonical_board_cache_.data()));
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

  void undoLastAction() {
    game_.UndoLastAction();
  }

  std::string actionToString(val a) const {
    XqA action{
      a["from"].as<uint8_t>(),
      a["to"].as<uint8_t>()
    };
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
    auto state = serializer_.SerializeCurrentState(g.get_game(), history.View());
    
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
    .function("currentPositionRepeatCount", &XqGameJs::currentPositionRepeatCount)
    .function("boardReadableString", &XqGameJs::boardReadableString)
    .function("getBoard", &XqGameJs::getBoard)
    .function("canonicalBoard", &XqGameJs::canonicalBoard)
    .function("validActions", &XqGameJs::validActions)
    .function("applyValidActionByIndex", &XqGameJs::applyValidActionByIndex)
    .function("undoLastAction", &XqGameJs::undoLastAction)
    .function("actionToString", &XqGameJs::actionToString)
    .function("actionFromString", &XqGameJs::actionFromString)
    ;
    
  class_<XqSerializerJs>("XqSerializerJs")
    .constructor<>()
    .function("serializeCurrentState", &XqSerializerJs::serializeCurrentState)
    .function("serializePolicyOutput", &XqSerializerJs::serializePolicyOutput)
    .function("getAugmentationStats", &XqSerializerJs::getAugmentationStats)
    ;
}
