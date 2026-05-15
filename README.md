# Xiang Qi

C++ implementation of Xiang Qi (Chinese Chess) game logic for the [alpha-zero-api](https://github.com/shuyangsun/alpha-zero-api) AlphaZero training and inference framework.

## Getting Started

### Building the Game Engine (C++)

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

### Running the GUI (Browser)

The repository includes a web-based GUI built with [TanStack Start](https://tanstack.com/start) and React. It relies on the game engine compiled to WebAssembly (WASM).

```bash
cd gui

# 1. Build the WASM engine (requires Docker to use the Emscripten image)
bun run wasm:build

# 2. Install frontend dependencies
bun install

# 3. Start the development server
bun run dev
```

Open `http://localhost:5173` to play Xiang Qi against yourself or inspect the `[Debug Panel]` output.

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
