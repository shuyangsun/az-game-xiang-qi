# Xiang Qi Web GUI

Browser-based self-play and debug UI for the C++ `XqGame` engine. It loads a
WebAssembly build of the engine (`public/wasm/xq.js` + `xq.wasm`) so the
client-side game logic, serializer round-trip, and inference-time
augmentations match the C++ trainer exactly. Out of the box you play Xiang Qi
against yourself in a single-page app and inspect the same signals the REPL
prints each turn.

- **Design doc**: [`../memory/gui_design.md`](../memory/gui_design.md) — what
  this GUI is for, the two audiences (in-repo debugging vs. upstream component
  reuse), and the `GameKit` contract.
- **Repo overview**: [`../README.md`](../README.md) — building the C++ engine
  and the top-level "Running the GUI" quick start.
- **Stack**: React 19 + TanStack Start/Router, Tailwind CSS v4, shadcn-style UI
  primitives. Package manager: `bun`.

## Prerequisites

- **Docker** — `wasm:build` runs the engine cross-compile inside the
  `emscripten/emsdk:latest` image, so you don't need a local Emscripten
  toolchain.
- **bun** — installs dependencies and runs every script.

## Getting started

```bash
# from gui/

# 1. Build the WASM engine -> public/wasm/xq.js + public/wasm/xq.wasm
bun run wasm:build

# 2. Install frontend dependencies
bun install

# 3. Start the dev server (http://localhost:5173)
bun run dev
```

`bun run wasm:build` shells out to `docker run ... emscripten/emsdk:latest` and
runs `emcmake cmake -S . -B build/wasm -DCMAKE_BUILD_TYPE=Release` to build the
`xq_wasm` target. That target's `POST_BUILD` step (see
[`../src/wasm/CMakeLists.txt`](../src/wasm/CMakeLists.txt)) copies the emitted
`xq.js` and `xq.wasm` into `public/wasm/`, where the engine loader fetches them
at runtime (default URLs `/wasm/xq.js` and `/wasm/xq.wasm`). Re-run `wasm:build`
whenever the C++ engine or its embind bindings (`../src/wasm/bindings.cc`)
change.

## Scripts

All scripts are defined in [`package.json`](./package.json):

| Script       | Purpose                                                                          |
| ------------ | -------------------------------------------------------------------------------- |
| `wasm:build` | Cross-compile the C++ engine via Docker/Emscripten into `public/wasm/xq.{js,wasm}`. |
| `dev`        | Vite dev server at `http://localhost:5173`.                                      |
| `build`      | Production build (Vite).                                                          |
| `serve`      | Preview the production build (`vite preview`).                                    |
| `test`       | Run the Vitest suite once (`vitest run`).                                         |
| `lint`       | Run ESLint.                                                                       |
| `format`     | `prettier --write . && eslint --fix`.                                            |
| `check`      | `prettier --check .` (CI-friendly, no writes).                                   |

## Source layout

- `src/engine/` — the WASM engine wrapper. `xq-wasm.ts` loads/caches the module
  (`loadXqWasm`), `xq-game.ts` wraps the embind classes (`XqGameWrapper`
  implementing `XqEngine`), `useXqGame.ts` is the React hook (`useXqGame`) that
  drives a game, `types.ts` holds `Snapshot` / `Action` / `Player` /
  `MoveProvider` / `DebugProbe`, and `index.ts` re-exports the public surface.
- `src/components/xiangqi/` — presentational game components: `XiangQiBoard`,
  `GameStatusBar`, `ControlBar`, `MoveList`, plus the debug surfaces
  `DebugPanel`, `AugmentationsGrid`, `DebugStatTiles`, and `MiniBoard`
  (barrel-exported from `index.ts`).
- `src/components/ui/` — shadcn-style primitives (`button.tsx`, `card.tsx`,
  `badge.tsx`); `src/components/ThemeToggle.tsx` toggles light/dark.
- `src/kit.ts` — the **GameKit** entry point. It exports `xqGameKit` (with pure
  `actionToString` / `actionFromString` / `encodeSnapshotForServer` helpers and
  the `kit-components.tsx` wrappers) so the upstream alpha-zero shell can consume
  this game through a `#game-kit` path alias. See
  [`../memory/gui_design.md`](../memory/gui_design.md) for the contract.
- `src/routes/` — file-based routes: `__root.tsx` (HTML shell) and `index.tsx`
  (the `Home` self-play page). `router.tsx` wires up the TanStack router;
  `routeTree.gen.ts` is generated.
- `src/styles.css` — Tailwind v4 entry and theme tokens.

## Debug panel

The self-play page renders a collapsible **Augmented variants** debug panel
([`components/xiangqi/DebugPanel.tsx`](./src/components/xiangqi/DebugPanel.tsx))
plus action-count and serialization-round-trip stat tiles. These reflect the
live `DebugProbe` computed from the engine each move (via the `XqDebugProbeJs`
embind binding), so they match the REPL's serializer / augmenter output. If the
WASM module was built without that binding, the panel shows a "Debug probe
unavailable" hint prompting a `bun run wasm:build`.

The debug UI is gated by an env flag read in
[`src/routes/index.tsx`](./src/routes/index.tsx):

```
VITE_XQ_HIDE_DEBUG_PANEL=1   # hide the debug panel and stat tiles
```

When `VITE_XQ_HIDE_DEBUG_PANEL` is set (truthy), the stat tiles and `DebugPanel`
are not rendered. Leave it unset for the default debugging view.

## Tooling

- **Styling**: Tailwind CSS v4 (`@tailwindcss/vite`), with shadcn-style
  components under `src/components/ui/`.
- **Routing**: TanStack Router file-based routes under `src/routes/`.
- **Lint/format**: ESLint (`@tanstack/eslint-config`) and Prettier — see the
  `lint` / `format` / `check` scripts above.
- **Tests**: Vitest (e.g. [`src/kit.test.ts`](./src/kit.test.ts)).
