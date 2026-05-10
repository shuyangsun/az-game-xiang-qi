# Xiang Qi

**TODO(TASK-UPDATE-README)**: add a one-sentence game description for Xiang Qi AlphaZero implementation.

## Getting Started

```bash
# Debug
cmake --preset debug && cmake --build --preset debug

# Release
cmake --preset release && cmake --build --preset release

# Run interactive terminal REPL (play yourself, prints serializer
# round-trip and augmentation debug info each turn)
./build/debug/xq    # Debug build
./build/release/xq  # Release build
```

## Tests

```bash
# Build and run tests (debug)
cmake --preset debug-test \
  && cmake --build --preset debug-test \
  && ctest --preset debug-test

# Build and run tests (release)
cmake --preset release-test \
  && cmake --build --preset release-test \
  && ctest --preset release-test
```
