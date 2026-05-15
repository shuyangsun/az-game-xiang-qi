# GUI Design

Design for the in-repo Xiang Qi GUI under [`gui/`](../gui/). The GUI
serves two audiences:

1. **In this repo (debugging)**: a developer plays Xiang Qi against
   themself in the browser, exercising `XqGame` end-to-end and
   inspecting the same serializer/augmenter signals the REPL prints
   (see [main_binary.md](./main_binary.md)).
2. **In the upstream AlphaZero repo (component reuse)**: the same
   board renderer and game-state primitives are consumed as a
   "game kit" so a human can play against the trained model. The
   model itself is CUDA-only and far too large for the browser, so
   the AlphaZero repo runs a C++ inference daemon
   (`alphazero_serve`) and plugs a `fetch`-based `MoveProvider`
   into `useXqGame` for the AI side. See
   [alpha-zero/docs/gui_design.md](../../alpha-zero/docs/gui_design.md)
   for the consuming side, including the `GameKit` interface this
   repo conforms to.

Both audiences must work from the **same** `XqGame` C++
implementation — duplicating the rules in TypeScript is a
non-starter (drift between trainer and UI silently breaks
training validation).

## Contents

- [Goals and non-goals](#goals-and-non-goals)
- [Architecture overview](#architecture-overview)
- [C++ ↔ browser bridge (WASM)](#c--browser-bridge-wasm)
- [TypeScript engine wrapper](#typescript-engine-wrapper)
- [UI components](#ui-components)
- [Self-play page (this repo)](#self-play-page-this-repo)
- [Reuse contract for the AlphaZero repo](#reuse-contract-for-the-alphazero-repo)
- [Build, dev, and test workflow](#build-dev-and-test-workflow)
- [Open questions](#open-questions)

## Goals and non-goals

### Goals

- Single source of truth for game logic: `XqGame` in C++, exposed to
  the browser via WebAssembly.
- A pure presentational `<XiangQiBoard>` that knows nothing about the
  engine or move source — it takes a board snapshot + interaction
  callbacks.
- A `useXqGame()` hook that wraps the WASM module and surfaces the
  same observers the REPL uses (`CurrentPlayer`, `CurrentRound`,
  `ValidActionsInto`, `IsOver`, `GetScore`, `LastAction`,
  `CurrentPositionRepeatCount`, `BoardReadableString`,
  `ActionToString`, `ActionFromString`).
- A debug panel that mirrors the REPL's `[debug]` lines: serializer
  state vector length, policy vector length, round-trip match,
  augmentation variant counts.
- The board renderer + engine hook ship as a self-contained subtree
  that the AlphaZero repo can drop in without pulling the rest of
  this app.

### Non-goals

- Multiplayer / networked play.
- Persistence across reloads (game state is in-memory only).
- Any AI or move suggestion in this repo's UI — that lives in the
  AlphaZero repo and consumes this GUI as a component.
- Pretty Chinese-Chess-style piece artwork; first cut uses
  Chinese-character glyphs on coloured discs (`帥/將`, `馬/傌`, …).
- Mobile-first responsive layout. Desktop-first; the board scales
  but secondary panels can stack vertically below a breakpoint.

## Architecture overview

```text
                ┌──────────────────────────────┐
                │  TanStack Start app (gui/)   │
                │                              │
                │  routes/index.tsx            │
                │   └─ <SelfPlayPage>          │
                │        ├─ <XiangQiBoard>     │  ◀── pure view
                │        ├─ <GameStatusBar>    │
                │        ├─ <MoveList>         │
                │        ├─ <ControlBar>       │
                │        └─ <DebugPanel>       │  ◀── this repo only
                │                              │
                │  engine/useXqGame.ts         │  ◀── React hook
                │   └─ engine/xq-wasm.ts       │  ◀── thin TS wrapper
                └──────────────┬───────────────┘
                               │ WASM module load (one-time)
                               ▼
                ┌──────────────────────────────┐
                │  xq.wasm + xq.js (Emscripten)│
                │   └─ src/wasm/bindings.cc    │  ◀── Embind shim
                │        └─ XqGame, XqSerializer, …
                └──────────────────────────────┘
```

The C++ game lives unchanged. A new CMake target (`xq_wasm`, gated on
the Emscripten toolchain) compiles `XqGame` plus a small Embind
binding file into a `.js` + `.wasm` pair that Vite serves from
`gui/public/wasm/` (or imports directly via `?url`).

The TS engine wrapper hides Embind details behind a small POJO API
(`createGame()`, `applyAction(idx)`, …). React components only see
plain values and callbacks.

## C++ ↔ browser bridge (WASM)

### Why WASM (vs. server function or TS reimplementation)

- **vs. TanStack Start server function**: a server roundtrip per move
  is fine for self-play, but the upstream AlphaZero repo will embed
  this GUI without our server. Keeping the engine client-side means
  the component is genuinely portable — only the bundled `xq.wasm`
  has to ship.
- **vs. TS reimplementation**: forks the source of truth.
  `ValidActions` ordering, `PolicyIndex`, `CanonicalAction`, and the
  compact-policy slot ordering are all load-bearing for trained
  weights (see [api_contract.md](./api_contract.md) "Compact policy
  head ordering"). A second implementation in TS will drift; when it
  drifts, the network's "valid moves" list and the GUI's "valid
  moves" list disagree silently.

### New C++ target

Add a new file `src/wasm/bindings.cc` and a new CMake target. Build is
opt-in and only runs under the Emscripten toolchain (so native debug /
release builds are untouched).

```text
src/
  wasm/
    bindings.cc       ◀── new: Embind shim
    CMakeLists.txt    ◀── new: target xq_wasm, only when EMSCRIPTEN
```

The target's output (`xq.js`, `xq.wasm`) is copied into
`gui/public/wasm/` by a CMake `POST_BUILD` step or a top-level script
(see [Build, dev, and test workflow](#build-dev-and-test-workflow)).

### Embind surface

The shim exposes a small JS-friendly class. **It must not** add new
C++ logic; every method is a thin forward to `XqGame`.

```cpp
class XqGameJs {
  XqGameJs();                                  // standard start, Red to move
  explicit XqGameJs(bool starting_player);     // false=Red, true=Black

  // Observers — matching the REPL's per-round prints.
  uint32_t  currentRound() const;
  bool      currentPlayer() const;             // false=Red, true=Black
  std::optional<bool>     lastPlayer() const;
  std::optional<XqAJs>    lastAction() const;
  bool      isOver() const;
  float     getScore(bool player) const;
  uint8_t   currentPositionRepeatCount() const;
  std::string boardReadableString() const;

  // Board: returns a Uint8Array view (signed bytes reinterpreted).
  // 90 entries, row-major. JS reads it via HEAP8.subarray.
  emscripten::val getBoard() const;            // Int8Array of length 90

  // Canonical-frame board for the debug panel toggle.
  emscripten::val canonicalBoard() const;

  // Legal actions — returns an array of {from, to} pairs.
  emscripten::val validActions() const;

  // Mutation. Action is given as the index into validActions(); the
  // shim looks it up to remove any chance of the JS side fabricating
  // an action XqGame would not have produced.
  void applyValidActionByIndex(uint32_t idx);
  void undoLastAction();

  // String I/O.
  std::string actionToString(XqAJs a) const;
  // Returns {ok, action, errorCode}. The action is only meaningful
  // when ok is true.
  emscripten::val actionFromString(std::string s) const;
};

// Optional, behind a debug flag — used by <DebugPanel>.
class XqSerializerJs {
  emscripten::val serializeCurrentState(const XqGameJs& g);
  emscripten::val serializePolicyOutput(const XqGameJs& g, /* probe */);
};
```

Notes:

- **No `from`/`to` arithmetic on the JS side beyond cell indexing.**
  The compact policy ordering, `PolicyIndex`, and `CanonicalAction`
  stay in C++. JS gets opaque `{from, to}` byte pairs and the
  per-call `validActions()` ordering.
- `applyValidActionByIndex(i)` keeps the JS side honest:
  `XqGame::ApplyActionInPlace` does **not** validate
  ([game.h](../include/xq/game.h) lines 339–355), so the shim must
  only accept actions returned by `ValidActionsInto`.
- The shim does **not** persist `XqGame` instances across
  module-load boundaries; one game per page load is enough for the
  self-play debug use case.

### Memory shape

`sizeof(XqGame)` is ~3.7 KiB (see
[game_design.md](./game_design.md) "Private state"). A single
instance is fine. The Emscripten heap is also small —
no `RingBuffer<XqGame>` is allocated because
`XqGame::kHistoryLookback == 0`.

## TypeScript engine wrapper

```text
gui/src/engine/
  xq-wasm.ts         // module-load helper, returns the Embind module
  xq-game.ts         // POJO wrapper: createGame(), engine methods
  useXqGame.ts       // React hook: state + memoized callbacks
  types.ts           // Action, BoardCell, Player, etc.
```

`xq-game.ts` exposes a small POJO façade so React components do not
touch Embind handles directly:

```typescript
type Player = "red" | "black";
type Action = { from: number; to: number }; // both 0..89
type BoardCell = number; // -7..-1, 0, 1..7

interface XqEngine {
  // Snapshot of the latest state. Replaced (not mutated) after each
  // applyAction/undo so React's reference equality drives re-renders.
  readonly snapshot: {
    board: Int8Array; // length 90, row-major
    currentPlayer: Player;
    currentRound: number;
    lastPlayer: Player | null;
    lastAction: Action | null;
    validActions: Action[]; // engine ordering, do not re-sort
    isOver: boolean;
    score: { red: number; black: number };
    repeatCount: number; // 1..3
    inCheck: boolean; // derived: see below
  };

  // Mutations. Throw if the action is not in validActions.
  applyAction(action: Action): void;
  undoLastAction(): void;
  reset(startingPlayer?: Player): void;

  // String I/O.
  actionToString(a: Action): string;
  actionFromString(s: string): Action | { error: string };
}
```

`inCheck` is **not** in the C++ API today (it's implicit in
`ValidActionsInto`). For the GUI's check indicator we either:

- Add a `bool IsInCheck() const noexcept` observer to `XqGame` (cheap
  — uses the same threat-detection logic as `ValidActionsInto`'s
  legality filter). Recommended.
- Or derive it in JS by checking `validActions.length == 0 &&
!isOver` plus a check probe — fragile, skip.

`useXqGame()` is a thin hook that owns one `XqEngine`, exposes
`snapshot`, and returns memoized `applyAction` / `undoLastAction` /
`reset` callbacks. A monotonically incrementing version counter in
local state forces re-render on mutation so we can keep the C++
instance long-lived (no per-move alloc).

## UI components

All under `gui/src/components/xiangqi/`. The board and pieces are
**presentational only** — they take props and emit callbacks. The
self-play page wires them to `useXqGame`.

### `<XiangQiBoard>`

The board itself.

Props (sketch):

```typescript
{
  board: Int8Array;                    // 90 cells
  orientation: "red-bottom" | "black-bottom";
  selectedCell: number | null;         // 0..89 or null
  legalTargets: ReadonlySet<number>;   // destination cells from selectedCell
  lastMove: Action | null;             // for from/to highlight
  inCheckCell: number | null;          // general's cell when in check
  onCellClick(cell: number): void;
  pieceStyle?: "char" | "icon";        // default "char"
}
```

Visual checklist:

- 9 vertical files × 10 horizontal ranks; pieces sit on
  intersections. Coordinate labels `a..i` along the bottom and
  `0..9` along the side, matching `ActionToString`.
- The river labelled between rows 4 and 5 (e.g., "楚河" / "漢界").
- The two palaces drawn with their `X` diagonals.
- Pieces are circular tokens with a Chinese glyph: `帥仕相馬車砲兵`
  for Red (warm colour) and `將士象傌俥炮卒` for Black (cool
  colour). Magnitude → glyph mapping comes from
  [board_encoding.md](./game_design_details/board_encoding.md).
- Selected piece: solid ring.
- Legal-move targets: small dot if empty cell, hollow ring if
  capture.
- Last move (`lastMove.from`, `lastMove.to`): subtle filled square
  background under each.
- In-check general: pulsing red glow on `inCheckCell`.

### `<GameStatusBar>`

A single-line strip showing:

- Side to move (with colour swatch and Chinese label 紅/黑).
- Round number.
- Repeat count, only when `> 1` (e.g., "Position seen 2× — one more
  draws").
- Check indicator.
- Game-over banner with score when `isOver`.

### `<MoveList>`

Scrollable list of moves played. Each row is `[round].
ActionToString(action)` (e.g., `"1. b0-c2"`). Latest row is anchored
at the bottom and auto-scrolls. Clicking a row selects it
(read-only — undo via the control bar). For the AlphaZero embedding
this can be hidden via a prop.

### `<ControlBar>`

Buttons:

- **New game** — opens a small popover with `Red starts` /
  `Black starts` and confirms with `reset(startingPlayer)`.
- **Undo** — disabled when `currentRound === 0`. Calls
  `undoLastAction`. Pure debug feature; the AlphaZero embedding
  hides it.
- **Flip board** — toggles `orientation`. Local UI state, does not
  touch the engine.

### `<DebugPanel>` (this repo only)

Mirrors the REPL's `[debug]` lines so the GUI is genuinely useful
for spot-checking serializer regressions (the entire reason
[main_binary.md](./main_binary.md) exists).

Sections:

1. **Raw board codes** — 10×9 grid of the signed-byte values from
   `getBoard()`. Toggle to `canonicalBoard()` view.
2. **Valid actions** — count + searchable list of
   `ActionToString` strings. Hovering an entry highlights its
   `from`→`to` arrow on the board.
3. **Selected action** — when one is hovered: its `PolicyIndex`,
   its canonical-frame `(from, to)`, and where it would sort in
   the compact policy ordering.
4. **Serialization probe** — calls `XqSerializerJs.serializeCurrentState`
   with the empty Markov history and `serializePolicyOutput` on a
   uniform-`pi` probe. Prints state vector length, policy vector
   length, and the deserializer round-trip status. Identical
   semantics to `PrintSerializationDebug` in
   [main.cc](../src/main.cc) lines 116–163.
5. **Augmentation** — variant count and per-variant action count
   from `XqInferenceAugmenter::Augment`. Identical to
   `PrintAugmentationDebug` in [main.cc](../src/main.cc) lines
   165–185.

The debug panel is gated by an env flag (e.g.,
`VITE_XQ_DEBUG_PANEL=1`) so the AlphaZero embedding does not
accidentally ship serializer probes.

## Self-play page (this repo)

`gui/src/routes/index.tsx` becomes the self-play page (replacing
the placeholder `<Home>`). Layout:

```text
┌────────────────────────────────────────────────────────────┐
│  GameStatusBar                                              │
├────────────────────────────────────────┬───────────────────┤
│                                        │  ControlBar       │
│            XiangQiBoard                │  MoveList         │
│                                        │                   │
├────────────────────────────────────────┴───────────────────┤
│  DebugPanel (collapsible, debug build only)                 │
└────────────────────────────────────────────────────────────┘
```

Click flow:

1. Page mounts → `useXqGame()` loads the WASM module (one-time).
2. User clicks a cell.
   - If the cell holds a piece of `currentPlayer`, set
     `selectedCell` and derive `legalTargets` by filtering
     `validActions` for `from === cell`.
   - If the cell is in `legalTargets`, call `applyAction({from:
selectedCell, to: cell})`, then clear `selectedCell`.
   - Otherwise clear `selectedCell`.
3. After every mutation: status bar, move list, debug panel and
   board re-render from the new snapshot.
4. When `snapshot.isOver`, show a `<GameOverDialog>` with the
   winner / draw reason and a "New game" button.

Keyboard shortcuts (debug ergonomics): `u` for undo, `n` for new
game, `f` for flip board, `Esc` to clear selection, `?` for a
help overlay listing the action notation.

## Reuse contract for the AlphaZero repo

The AlphaZero repo imports this UI as a **game kit** — one
TypeScript module that bundles the engine hook, presentational
components, action codec, and a server wire-format encoder behind a
single `GameKit` interface defined upstream
([alpha-zero/docs/gui_design.md](../../alpha-zero/docs/gui_design.md),
"Game kit: the generic seam"). To avoid the classic "extract it
later" trap, lay it out for reuse from day one:

- **One entry point**: `gui/src/kit.ts` exports a `GameKit`-
  conforming object composing the engine and component modules
  listed in this doc. The AlphaZero repo resolves it through a
  tsconfig path alias (`#game-kit`) pointing at this file. To
  swap games on that side, the AlphaZero repo rewrites the alias
  to a sibling game's kit; nothing in this repo needs to know.
- **Public exports** live under `gui/src/components/xiangqi/index.ts`,
  `gui/src/engine/index.ts`, and `gui/src/kit.ts`. Anything not
  re-exported there is a private implementation detail.
- **AI move source**: `useXqGame()` accepts an optional
  `moveProvider: { red?: MoveProvider; black?: MoveProvider }`.
  A `MoveProvider` returns a `Promise<Action>` given the current
  snapshot. Default for both sides is "human via clicks". The
  AlphaZero repo passes a `MoveProvider` that `fetch`-es its
  `alphazero_serve` daemon (CUDA-loaded model + MCTS) for the AI
  side and leaves the other as human. The hook stays agnostic
  about where the action came from.
- **Server wire encoder**: `kit.ts` exports
  `encodeSnapshotForServer(snapshot) -> { board, current_player,
  current_round, history }`. The AlphaZero C++ daemon rehydrates
  an `XqGame` from this payload via the same `bindings.cc` shim
  used by the browser, keeping one source of truth for the
  representation. `XqGame::kHistoryLookback == 0` means `history`
  is always `[]` for XQ today.
- **Debug panel exclusion**: board, status bar, and move list
  have no dependency on the debug panel. `kit.ts` does **not**
  expose `<DebugPanel>` in its `GameKit` object, so the AlphaZero
  shell cannot accidentally import it.
- **WASM module path** is configurable
  (`xq-wasm.ts: loadXqWasm({ wasmUrl, jsUrl })`) so the consuming
  app can serve `xq.wasm` from wherever it likes. The kit's
  defaults are `'/wasm/xq.wasm'` and `'/wasm/xq.js'`.
- **Styling**: components accept `className` overrides and read
  sizing via CSS variables (`--xq-board-cell-size`,
  `--xq-piece-size`, …). No hard pixel sizes inside components.

### Remaining reuse-contract work

The kit-export contract above is partially in place (`moveProvider`,
`loadXqWasm({wasmUrl, jsUrl})`, and the component `index.ts`
barrels already exist). The remaining items, all additive:

1. **Add `gui/src/kit.ts`.** Composes `useXqGame`,
   `<XiangQiBoard>`, `<GameStatusBar>`, `<MoveList>`,
   `actionToString`, `actionFromString`, and a new
   `encodeSnapshotForServer` into one `GameKit`-conforming object.
2. **Accept `className` props** on `<XiangQiBoard>`,
   `<GameStatusBar>`, and `<MoveList>`. Today these hard-code
   Tailwind classes; the AlphaZero shell needs layout-level
   overrides.
3. **Read cell size from `--xq-board-cell-size`** instead of the
   `const cellSize = 50` constant in `<XiangQiBoard>`. Lets the
   AlphaZero shell shrink the board for its side-panel-heavy
   `<GamePlayPage>` layout.

## Build, dev, and test workflow

### One-time setup

- Install Emscripten (or use the `emsdk` Docker image). Document the
  expected version in this design doc once we pin it.
- Add a CMake preset `wasm` that selects the Emscripten toolchain
  and builds only the `xq_wasm` target.

### Top-level script

Add a `bun run wasm:build` script in `gui/package.json` that:

1. Invokes `cmake --preset wasm && cmake --build --preset wasm`.
2. Copies `build/wasm/src/wasm/xq.{js,wasm}` into
   `gui/public/wasm/`.

`bun run dev` is unchanged — it picks up the prebuilt
`gui/public/wasm/xq.wasm`. CI runs `wasm:build` before
`bun run build`. We can add `bun run dev:full` later that chains
both if the manual step becomes annoying.

### Tests

- **Vitest** (`gui/`): the engine wrapper is testable headlessly
  by loading the same WASM module. Cover: `applyAction` rejects
  non-legal actions; `undoLastAction` reverses; `actionFromString`
  round-trips; `repeatCount` reaches 3 on a contrived sequence;
  game-over score values match `XqGame::GetScore` for a known
  checkmate.
- **Component tests** (Vitest + Testing Library): click flows on
  `<XiangQiBoard>` with a stubbed engine — no WASM needed for
  these.
- **Visual smoke**: a Storybook-style route under
  `gui/src/routes/__debug/` (gated on env flag) that renders the
  board for a fixed sequence of positions. Manual verification
  only; not in CI.

## Open questions

1. **Embind vs. raw exports**: Embind is ergonomic but adds ~50 KiB
   of glue. For a board that fits on one screen this is fine, but
   the AlphaZero repo will also ship its own WASM (the network).
   Worth measuring once we have the first build.
2. **`IsInCheck` observer**: do we add it to `XqGame` (clean) or
   compute it in the WASM shim (decoupled but duplicates threat
   logic)? Lean toward adding it to `XqGame`.
3. **Piece artwork**: ship character glyphs first; consider SVG
   piece icons later if the AlphaZero embedding wants a more
   polished feel.
4. **Routing in the AlphaZero repo embedding**: the AlphaZero repo
   may not use TanStack Router. The component subtree must work
   without `createFileRoute` — only `routes/index.tsx` knows about
   the router.
5. **Threading**: do we need a Web Worker for the WASM module? Not
   for self-play (every call is microsecond-scale), but the
   AlphaZero repo's network inference will want one. Keep the
   engine wrapper synchronous for now; revisit when AI integration
   lands.
