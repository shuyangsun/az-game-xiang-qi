#include <cctype>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <sstream>
#include <string>
#include <string_view>

#include "include/xq/game.h"
#include "src/xq/game/internal.h"

namespace az::game::xq {

namespace {

using ::az::game::xq::internal::Col;
using ::az::game::xq::internal::Idx;
using ::az::game::xq::internal::PieceType;
using ::az::game::xq::internal::Row;

constexpr char kColLetters[] = "abcdefghi";

constexpr char PieceGlyph(int8_t code) noexcept {
  // Uppercase = Red, lowercase = Black. Letter chosen for the
  // English mnemonic used in the rules doc.
  switch (code) {
    case 1: return 'G';   // Red General
    case 2: return 'A';   // Red Advisor
    case 3: return 'E';   // Red Elephant
    case 4: return 'H';   // Red Horse
    case 5: return 'R';   // Red chaRiot
    case 6: return 'C';   // Red Cannon
    case 7: return 'S';   // Red Soldier
    case -1: return 'g';
    case -2: return 'a';
    case -3: return 'e';
    case -4: return 'h';
    case -5: return 'r';
    case -6: return 'c';
    case -7: return 's';
    default: return '.';
  }
}

std::expected<uint8_t, XqError> ParseCell(char col_ch, char row_ch) noexcept {
  const char col_lower =
      static_cast<char>(std::tolower(static_cast<unsigned char>(col_ch)));
  if (col_lower < 'a' || col_lower >= static_cast<char>('a' + kBoardCols)) {
    return std::unexpected(XqError::kInvalidActionFormat);
  }
  if (row_ch < '0' || row_ch >= static_cast<char>('0' + kBoardRows)) {
    return std::unexpected(XqError::kInvalidActionFormat);
  }
  return ::az::game::xq::internal::Idx(
      static_cast<uint8_t>(row_ch - '0'),
      static_cast<uint8_t>(col_lower - 'a'));
}

std::string_view Trim(std::string_view input) noexcept {
  while (!input.empty() &&
         std::isspace(static_cast<unsigned char>(input.front())) != 0) {
    input.remove_prefix(1);
  }
  while (!input.empty() &&
         std::isspace(static_cast<unsigned char>(input.back())) != 0) {
    input.remove_suffix(1);
  }
  return input;
}

}  // namespace

std::string XqGame::BoardReadableString() const noexcept {
  std::ostringstream out;
  // Print rows top-down so Black is at the top, Red at the bottom —
  // matches how Xiang Qi diagrams are printed in convention.
  for (int r = static_cast<int>(kBoardRows) - 1; r >= 0; --r) {
    out << r << ' ';
    for (uint8_t c = 0; c < kBoardCols; ++c) {
      out << ' ' << PieceGlyph(board_[Idx(static_cast<uint8_t>(r), c)]);
    }
    out << '\n';
  }
  out << "  ";
  for (uint8_t c = 0; c < kBoardCols; ++c) {
    out << ' ' << kColLetters[c];
  }
  return out.str();
}

XqResult<XqA> XqGame::ActionFromString(
    std::string_view action_str) const noexcept {
  const std::string_view trimmed = Trim(action_str);
  if (trimmed.size() != 5 || trimmed[2] != '-') {
    return std::unexpected(XqError::kInvalidActionFormat);
  }
  const std::expected<uint8_t, XqError> from_cell =
      ParseCell(trimmed[0], trimmed[1]);
  if (!from_cell.has_value()) {
    return std::unexpected(XqError::kInvalidActionFromCell);
  }
  const std::expected<uint8_t, XqError> to_cell =
      ParseCell(trimmed[3], trimmed[4]);
  if (!to_cell.has_value()) {
    return std::unexpected(XqError::kInvalidActionToCell);
  }
  return XqA{*from_cell, *to_cell};
}

std::string XqGame::ActionToString(const XqA& action) const noexcept {
  if (action.from >= kBoardCells || action.to >= kBoardCells) {
    return "?\?-?\?";
  }
  std::string out;
  out.reserve(5);
  out.push_back(kColLetters[Col(action.from)]);
  out.push_back(static_cast<char>('0' + Row(action.from)));
  out.push_back('-');
  out.push_back(kColLetters[Col(action.to)]);
  out.push_back(static_cast<char>('0' + Row(action.to)));
  return out;
}

}  // namespace az::game::xq
