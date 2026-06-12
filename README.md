# Xiang Qi

C++ implementation of Xiang Qi (Chinese Chess) game logic for the [alpha-zero-api](https://github.com/shuyangsun/alpha-zero-api) AlphaZero training and inference framework.

## Project Memory

The primary design and retrieval corpus lives in [`memory/`](memory/). Start
with [`memory/README.md`](memory/README.md) for the indexed map to the API
contract, Xiang Qi rules, `XqGame` design, serializer and augmentation strategy,
GUI architecture, task history, and unit-test coverage.

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

The repository includes a web-based GUI built with [TanStack Start](https://tanstack.com/start) and React. It relies on the game engine compiled to WebAssembly (WASM), so the browser runs the exact same `XqGame` logic as the trainer. See [`gui/README.md`](gui/README.md) for the full GUI workflow and [`memory/gui_design.md`](memory/gui_design.md) for the design.

```bash
cd gui

# 1. Build the WASM engine (requires Docker to use the Emscripten image)
bun run wasm:build

# 2. Install frontend dependencies
bun install

# 3. Start the development server
bun run dev
```

Open `http://localhost:5173` to play Xiang Qi against yourself or inspect the **Augmented variants** debug panel.

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
