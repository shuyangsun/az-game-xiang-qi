// Xiang Qi GameKit entry point.
//
// The alpha-zero shell imports this module via a tsconfig path alias
// (`#game-kit`) to render Xiang Qi without depending on any other
// XQ-specific file. See alpha-zero/docs/gui_design.md > "Game kit:
// the generic seam" for the shape this conforms to.

import type {
  Action,
  Player,
  Snapshot,
  MoveProvider,
  XqEngine,
} from './engine'
import { useXqGame } from './engine/useXqGame'
import {
  XiangQiKitBoard,
  XiangQiKitMoveList,
  XiangQiKitStatusBar,
} from './kit-components'
import type {
  KitBoardProps,
  KitMoveListProps,
  KitStatusBarProps,
} from './kit-components'

// ---------------------------------------------------------------------------
// Generic GameKit contract (mirrors the upstream interface from
// alpha-zero/gui/src/engine-kit/types.ts). Kept here so this repo
// type-checks the kit independently; structural compatibility is what
// lets the shell import it.

export interface GameMeta {
  gameId: string
  displayName: string
  players: readonly [string, string]
  policySize: number
  serverWireVersion: 1
  // Cells of history the server needs alongside the current position.
  // Xiang Qi is Markov, so 0.
  historyLookback: number
}

export interface UseGameOptionsLike<TAction, TPlayer extends string> {
  startingPlayer?: TPlayer
  moveProvider?: Partial<Record<TPlayer, MoveProvider>>
  wasmConfig?: { wasmUrl?: string; jsUrl?: string }
  // TAction is reserved here so the upstream interface can specialise
  // moveProvider/applyAction; the XQ kit doesn't need it locally.
  __actionBrand?: TAction
}

export interface UseGameResult<TSnapshot, TAction, TPlayer extends string> {
  engine: XqEngine | null
  snapshot: TSnapshot | null
  error: Error | null
  applyAction: (a: TAction) => void
  undoLastAction: () => void
  reset: (startingPlayer?: TPlayer) => void
  isReady: boolean
}

export interface GameKit<TSnapshot, TAction, TPlayer extends string> {
  readonly meta: GameMeta

  useGame: (
    opts: UseGameOptionsLike<TAction, TPlayer>,
  ) => UseGameResult<TSnapshot, TAction, TPlayer>

  Board: React.ComponentType<KitBoardProps<TSnapshot, TAction>>
  StatusBar: React.ComponentType<KitStatusBarProps<TSnapshot, TPlayer>>
  MoveList: React.ComponentType<KitMoveListProps<TAction>>

  actionToString: (a: TAction) => string
  actionFromString: (s: string) => TAction | { error: string }

  encodeSnapshotForServer: (s: TSnapshot) => ServerStatePayload
}

// Shape that `gui/src/engine-kit/server-move-provider.ts` upstream
// spreads into the inference daemon's `/infer` request body.
export interface ServerStatePayload {
  board: number[] // length 90, row-major, signed int8 values 0..7 / -7..-1
  current_player: 'red' | 'black'
  current_round: number
}

// ---------------------------------------------------------------------------
// Pure action <-> string helpers. Mirrors XqGame::ActionToString /
// XqGame::ActionFromString in src/xq/game/string_conv.cc — the wire
// notation is `${file}${rank}-${file}${rank}` (file a..i, rank 0..9),
// and cell index = row * 9 + col.

const BOARD_COLS = 9
const BOARD_ROWS = 10
const BOARD_CELLS = BOARD_COLS * BOARD_ROWS

function colOf(idx: number): number {
  return idx % BOARD_COLS
}

function rowOf(idx: number): number {
  return Math.floor(idx / BOARD_COLS)
}

function cellLabel(idx: number): string {
  return `${String.fromCharCode(97 + colOf(idx))}${rowOf(idx)}`
}

function parseCell(label: string): number | null {
  if (label.length !== 2) return null
  const col = label.charCodeAt(0) - 97
  const row = label.charCodeAt(1) - 48
  if (col < 0 || col >= BOARD_COLS) return null
  if (row < 0 || row >= BOARD_ROWS) return null
  return row * BOARD_COLS + col
}

export function actionToString(a: Action): string {
  if (
    a.from < 0 ||
    a.from >= BOARD_CELLS ||
    a.to < 0 ||
    a.to >= BOARD_CELLS
  ) {
    return '??-??'
  }
  return `${cellLabel(a.from)}-${cellLabel(a.to)}`
}

export function actionFromString(s: string): Action | { error: string } {
  const trimmed = s.trim()
  if (trimmed.length !== 5 || trimmed[2] !== '-') {
    return { error: 'kInvalidActionFormat' }
  }
  const from = parseCell(trimmed.slice(0, 2))
  if (from === null) return { error: 'kInvalidActionFromCell' }
  const to = parseCell(trimmed.slice(3, 5))
  if (to === null) return { error: 'kInvalidActionToCell' }
  return { from, to }
}

// ---------------------------------------------------------------------------
// Server wire encoder. The daemon rebuilds an `XqGame` from this
// payload — see alpha-zero/docs/gui_design.md > "AI move provider".

export function encodeSnapshotForServer(s: Snapshot): ServerStatePayload {
  // Copy out of the Int8Array because JSON.stringify on a typed array
  // produces an object-keyed mess. Cells are -7..7, so number[] is fine.
  const board = new Array<number>(s.board.length)
  for (let i = 0; i < s.board.length; i++) board[i] = s.board[i]
  return {
    board,
    current_player: s.currentPlayer,
    current_round: s.currentRound,
  }
}

// ---------------------------------------------------------------------------
// The exported kit. The alpha-zero shell's `#game-kit` path alias
// resolves to this file.

export const xqGameKit: GameKit<Snapshot, Action, Player> = {
  meta: {
    gameId: 'xiang-qi',
    displayName: 'Xiang Qi',
    players: ['red', 'black'] as const,
    // Must match XqGame::kPolicySize (include/xq/game.h).
    policySize: 8100,
    serverWireVersion: 1,
    historyLookback: 0,
  },

  useGame: useXqGame,

  Board: XiangQiKitBoard,
  StatusBar: XiangQiKitStatusBar,
  MoveList: XiangQiKitMoveList,

  actionToString,
  actionFromString,

  encodeSnapshotForServer,
}

export default xqGameKit
