#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "alpha-zero-api/ring_buffer.h"
#include "include/xq/deserializer.h"
#include "include/xq/game.h"
#include "include/xq/inference.h"
#include "include/xq/serializer.h"
#include "include/xq/train.h"

namespace {

using ::az::game::api::Evaluation;
using ::az::game::api::RingBuffer;
using ::az::game::api::TrainingTarget;

using ::az::game::xq::XqA;
using ::az::game::xq::XqDeserializer;
using ::az::game::xq::XqError;
using ::az::game::xq::XqGame;
using ::az::game::xq::XqInferenceAugmenter;
using ::az::game::xq::XqP;
using ::az::game::xq::XqResult;
using ::az::game::xq::XqSerializer;
using ::az::game::xq::XqTrainingAugmenter;

// History buffer for the network input. Capacity is set by
// `XqGame::kHistoryLookback`; for Markov games the
// capacity is 0 and `View()` always returns an empty view.
using GameHistory =
    RingBuffer<XqGame, std::array<XqGame, XqGame::kHistoryLookback>>;

constexpr std::string_view kCmdActions = "actions";
constexpr std::string_view kCmdHelp = "help";
constexpr std::string_view kCmdQuit = "quit";

// underlying_type_t<E> may be `uint8_t` (== unsigned char), and operator<<
// on a char type prints a glyph instead of digits. Always promote to
// unsigned for printing.
unsigned ErrorCode(XqError e) noexcept {
  return static_cast<unsigned>(static_cast<std::underlying_type_t<XqError>>(e));
}

std::string Trim(std::string_view input) noexcept {
  std::size_t start = 0;
  while (start < input.size() &&
         std::isspace(static_cast<unsigned char>(input[start])) != 0) {
    ++start;
  }
  std::size_t end = input.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
    --end;
  }
  return std::string(input.substr(start, end - start));
}

// Player printing is templated so `if constexpr` discards non-matching
// branches at instantiation time. A non-template function would type-check
// every branch, which breaks for Player types that are neither bool nor
// integral nor stream-printable.
template <typename P>
std::string PlayerString(const P& player) {
  if constexpr (std::is_same_v<P, bool>) {
    return player ? "Player 2" : "Player 1";
  } else if constexpr (std::is_integral_v<P>) {
    std::ostringstream oss;
    oss << "Player " << static_cast<long long>(player);
    return oss.str();
  } else {
    std::ostringstream oss;
    oss << player;
    return oss.str();
  }
}

void PrintHelp(std::ostream& os) {
  os << "Commands:\n"
     << "  <action>   Take an action by typing its string form (the same\n"
     << "             form returned by ActionToString and accepted by\n"
     << "             ActionFromString).\n"
     << "  actions    List the current valid actions.\n"
     << "  help       Show this help message.\n"
     << "  quit       Exit the REPL.\n";
}

void PrintValidActions(std::ostream& os, const XqGame& game,
                       std::span<const XqA> actions) {
  os << "Valid actions (" << actions.size() << "):\n";
  for (std::size_t i = 0; i < actions.size(); ++i) {
    os << "  " << game.ActionToString(actions[i]) << "\n";
  }
}

bool ApproxEqual(float a, float b) noexcept {
  constexpr float kAbs = 1e-5F;
  constexpr float kRel = 1e-4F;
  const float diff = std::fabs(a - b);
  return diff <= kAbs || diff <= kRel * std::max(std::fabs(a), std::fabs(b));
}

void PrintSerializationDebug(std::ostream& os, const XqGame& game,
                             const XqSerializer& serializer,
                             const XqDeserializer& deserializer,
                             const GameHistory& history,
                             std::span<const XqA> actions) {
  const std::vector<float> nn_input =
      serializer.SerializeCurrentState(game, history.View());
  os << "[debug] state vector length:  " << nn_input.size() << "\n";

  std::vector<float> probs(actions.size(), 0.0F);
  if (!actions.empty()) {
    std::fill(probs.begin(), probs.end(),
              1.0F / static_cast<float>(actions.size()));
  }
  const TrainingTarget probe{0.5F, probs};
  const std::vector<float> nn_output =
      serializer.SerializePolicyOutput(game, probe);
  os << "[debug] policy vector length: " << nn_output.size() << "\n";

  if (nn_output.empty()) {
    os << "[debug] round-trip:           skipped "
       << "(SerializePolicyOutput returned an empty vector)\n";
    return;
  }

  const XqResult<Evaluation> roundtrip =
      deserializer.Deserialize(game, nn_output);
  if (!roundtrip.has_value()) {
    os << "[debug] round-trip:           failed (Deserialize error="
       << ErrorCode(roundtrip.error()) << ")\n";
    return;
  }
  const Evaluation& got = *roundtrip;
  bool match = got.probabilities.size() == probe.pi.size() &&
               ApproxEqual(got.value, probe.z);
  for (std::size_t i = 0; match && i < probe.pi.size(); ++i) {
    if (!ApproxEqual(got.probabilities[i], probe.pi[i])) {
      match = false;
    }
  }
  os << "[debug] round-trip:           " << (match ? "match" : "MISMATCH")
     << "\n";
  if (!match) {
    os << "[debug]   probe.z=" << probe.z << " got.value=" << got.value << "\n";
    os << "[debug]   probe.pi.size=" << probe.pi.size()
       << " got.probs.size=" << got.probabilities.size() << "\n";
  }
}

void PrintAugmentationDebug(std::ostream& os, const XqGame& game,
                            const XqInferenceAugmenter& inference,
                            const XqTrainingAugmenter& trainer,
                            std::span<const XqA> actions) {
  const std::vector<XqGame> augmented = inference.Augment(game);
  os << "[debug] inference variants:   " << augmented.size() << "\n";
  for (std::size_t i = 0; i < augmented.size(); ++i) {
    const std::vector<XqA> aug_actions = augmented[i].ValidActions();
    os << "[debug]   key=" << i << " actions=" << aug_actions.size() << "\n";
  }

  std::vector<float> probs(actions.size(), 0.0F);
  if (!actions.empty()) {
    std::fill(probs.begin(), probs.end(),
              1.0F / static_cast<float>(actions.size()));
  }
  const std::vector<std::pair<XqGame, TrainingTarget>> training =
      trainer.Augment(game, TrainingTarget{0.0F, probs});
  os << "[debug] training examples:    " << training.size() << "\n";
}

// Templated for the same reason PlayerString is: the bool branch calls
// GetScore(true)/GetScore(false), which only type-checks when Player is bool.
template <typename G>
void PrintFinalScores(std::ostream& os, const G& game) {
  using PlayerType = typename G::player_t;
  if constexpr (std::is_same_v<PlayerType, bool>) {
    os << "  " << PlayerString(false) << " score=" << game.GetScore(false)
       << "\n"
       << "  " << PlayerString(true) << " score=" << game.GetScore(true)
       << "\n";
  } else {
    const PlayerType cur = game.CurrentPlayer();
    os << "  " << PlayerString(cur) << " score=" << game.GetScore(cur) << "\n";
    const std::optional<PlayerType> last = game.LastPlayer();
    if (last.has_value()) {
      os << "  " << PlayerString(*last) << " score=" << game.GetScore(*last)
         << "\n";
    }
  }
}

bool FindMatchingValidAction(const XqGame& game, std::span<const XqA> actions,
                             const XqA& parsed, XqA* out) {
  const std::string parsed_str = game.ActionToString(parsed);
  for (const XqA& a : actions) {
    if (game.ActionToString(a) == parsed_str) {
      *out = a;
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  std::cout << "=== Xiang Qi REPL ===\n"
            << "Type \"help\" for commands, \"actions\" to list valid moves, "
            << "\"quit\" to exit.\n";

  XqGame game;
  GameHistory history;

  const XqSerializer serializer;
  const XqDeserializer deserializer;
  const XqInferenceAugmenter inference;
  const XqTrainingAugmenter trainer;

  while (true) {
    std::cout << "\n--- Round " << game.CurrentRound() << " ---\n"
              << "Current: " << PlayerString(game.CurrentPlayer()) << "\n";

    const std::optional<XqP> last_player = game.LastPlayer();
    const std::optional<XqA> last_action = game.LastAction();
    if (last_player.has_value() && last_action.has_value()) {
      std::cout << "Last:    " << PlayerString(*last_player) << " played "
                << game.ActionToString(*last_action) << "\n";
    }
    std::cout << "\n" << game.BoardReadableString() << "\n";

    if (game.IsOver()) {
      std::cout << "\nGame over.\n";
      PrintFinalScores(std::cout, game);
      return 0;
    }

    const std::vector<XqA> valid_actions = game.ValidActions();
    std::cout << "\nValid actions: " << valid_actions.size()
              << " (type \"actions\" to list)\n";

    PrintSerializationDebug(std::cout, game, serializer, deserializer, history,
                            valid_actions);
    PrintAugmentationDebug(std::cout, game, inference, trainer, valid_actions);

    while (true) {
      std::cout << "\n> " << std::flush;
      std::string raw;
      if (!std::getline(std::cin, raw)) {
        std::cout << "\n[EOF — exiting]\n";
        return 0;
      }
      const std::string typed = Trim(raw);
      if (typed.empty()) {
        continue;
      }
      if (typed == kCmdQuit) {
        return 0;
      }
      if (typed == kCmdHelp) {
        PrintHelp(std::cout);
        continue;
      }
      if (typed == kCmdActions) {
        PrintValidActions(std::cout, game, valid_actions);
        continue;
      }

      const XqResult<XqA> parsed = game.ActionFromString(typed);
      if (!parsed.has_value()) {
        std::cout << "Could not parse \"" << typed << "\" as an action "
                  << "(ActionFromString error=" << ErrorCode(parsed.error())
                  << "). Type \"actions\" to see what is allowed.\n";
        continue;
      }

      XqA chosen{};
      if (!FindMatchingValidAction(game, valid_actions, *parsed, &chosen)) {
        std::cout << "\"" << typed
                  << "\" parsed but is not a currently valid action. "
                  << "Type \"actions\" to see what is allowed.\n";
        continue;
      }

      // Push the current state to history before transitioning so the
      // serializer's view at the next round contains this round's state.
      // For Markov games (kHistoryLookback == 0) this is a no-op.
      history.Push(game);
      game.ApplyActionInPlace(chosen);
      break;
    }
  }
}
