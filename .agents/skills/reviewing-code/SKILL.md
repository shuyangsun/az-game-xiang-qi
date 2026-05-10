---
name: reviewing-code
description: Review C++ and CMake code to ensure code quality and consistency.
---

# Reviewing Code

## C++

Strictly follow Google's C++ style guide. It is okay to use the version you
already know from training time. There is no need to look it up on the
internet unless you do not remember certain details.

### Key things to remember

- Headers are `.h`, implementations are `.cc`.
- Use CamelCase for function and method names.
- We're using C++23, although the style guide does not recommend C++23 yet.
- Do not use the `auto` type. Use `using ...` in an anonymous namespace (`.cc`
  files only), and add aliases for commonly used custom types if the type name
  is too long.
- Throwing exceptions is strictly forbidden. Use `std::expected` to communicate
  possible error states. `try`/`catch` blocks are only permitted when a
  third-party library may throw, but the function should still return a result
  type. If `std::expected` makes the type name too long, create Abseil-style
  aliases for status and result types (e.g.,
  `enum class TttError {...};`,
  `using TttResult<T> = std::expected<T, TttError>;`, and
  `using TttStatus = TttResult<void>;`).
- Mark functions as `[nodiscard]` and `noexcept` whenever possible.
- Prefer private constructors and public static factory methods that return
  result types when construction can fail. Public constructors are fine for
  value-semantic types whose construction cannot fail (e.g., a game state
  satisfying `::az::game::api::Game`).
- Use `std::string_view` for immutable string parameters instead of
  `const std::string&`. Use `std::span` for immutable array parameters instead
  of `const std::vector<T>&`.
