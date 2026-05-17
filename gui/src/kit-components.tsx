// Generic-shape wrappers around the XQ presentational components.
// The kit (`kit.ts`) exposes these so the alpha-zero shell can render
// the game with snapshot+onAction props that don't leak XQ-specific
// piece selection / orientation state.

import { useState } from 'react'
import type { Action, DebugProbe, Player, Snapshot } from './engine'
import { GameStatusBar } from './components/xiangqi/GameStatusBar'
import { MoveList } from './components/xiangqi/MoveList'
import { XiangQiBoard } from './components/xiangqi/XiangQiBoard'
import type { Orientation } from './components/xiangqi/XiangQiBoard'
import {
  AugmentationsGrid,
  DebugPanel,
  MiniBoard,
  ActionCountTile,
  SerializationProbeTile,
  VariantCountTile,
} from './components/xiangqi'

// ---------------------------------------------------------------------------
// Board

export interface KitBoardProps<TSnapshot, TAction> {
  snapshot: TSnapshot
  onAction: (a: TAction) => void
  /** Layout-level className applied to the outermost wrapper. */
  className?: string
  /**
   * If true, render with black at the bottom. The kit defaults to
   * red-at-bottom regardless of whose turn it is.
   */
  flipped?: boolean
  /** Suppress click handling (e.g. while the AI is thinking). */
  disabled?: boolean
}

export function XiangQiKitBoard({
  snapshot,
  onAction,
  className,
  flipped,
  disabled,
}: KitBoardProps<Snapshot, Action>) {
  const [selectedCell, setSelectedCell] = useState<number | null>(null)

  const orientation: Orientation = flipped ? 'black-bottom' : 'red-bottom'

  const legalTargets = (() => {
    const targets = new Set<number>()
    if (selectedCell !== null) {
      for (const a of snapshot.validActions) {
        if (a.from === selectedCell) targets.add(a.to)
      }
    }
    return targets
  })()

  const inCheckCell = (() => {
    if (!snapshot.inCheck) return null
    const targetVal = snapshot.currentPlayer === 'red' ? 1 : -1
    for (let i = 0; i < snapshot.board.length; i++) {
      if (snapshot.board[i] === targetVal) return i
    }
    return null
  })()

  const handleCellClick = (cellIdx: number) => {
    if (disabled || snapshot.isOver) return

    if (legalTargets.has(cellIdx) && selectedCell !== null) {
      onAction({ from: selectedCell, to: cellIdx })
      setSelectedCell(null)
      return
    }

    const val = snapshot.board[cellIdx]
    const ownsPiece =
      (snapshot.currentPlayer === 'red' && val > 0) ||
      (snapshot.currentPlayer === 'black' && val < 0)
    setSelectedCell(ownsPiece ? cellIdx : null)
  }

  return (
    <XiangQiBoard
      board={snapshot.board}
      orientation={orientation}
      selectedCell={selectedCell}
      legalTargets={legalTargets}
      lastMove={snapshot.lastAction}
      inCheckCell={inCheckCell}
      onCellClick={handleCellClick}
      className={className}
    />
  )
}

// ---------------------------------------------------------------------------
// StatusBar

export interface KitStatusBarProps<TSnapshot, TPlayer extends string> {
  snapshot: TSnapshot
  /**
   * Side the human player is playing, if any. Reserved for future
   * "You are red"-style status text; unused by the XQ status bar
   * today, but present so the kit-level type stays parameterised.
   */
  humanPlayer?: TPlayer
  className?: string
}

export function XiangQiKitStatusBar({
  snapshot,
  className,
}: KitStatusBarProps<Snapshot, Player>) {
  return (
    <GameStatusBar
      currentPlayer={snapshot.currentPlayer}
      currentRound={snapshot.currentRound}
      repeatCount={snapshot.repeatCount}
      inCheck={snapshot.inCheck}
      isOver={snapshot.isOver}
      score={snapshot.score}
      className={className}
    />
  )
}

// ---------------------------------------------------------------------------
// MoveList
//
// The kit's `actionToString` is the canonical formatter — the shell
// is expected to pre-format actions before passing them here.

export interface KitMoveListProps<TAction> {
  moves: string[]
  /**
   * Optional structured action array parallel to `moves`. Lets a
   * future shell wire move-jumping or hover-preview affordances
   * without re-parsing strings.
   */
  actions?: ReadonlyArray<TAction>
  className?: string
}

export function XiangQiKitMoveList({
  moves,
  className,
}: KitMoveListProps<Action>) {
  return <MoveList moves={moves} className={className} />
}

// ---------------------------------------------------------------------------
// Debug surface
//
// Each piece is exposed so a consuming shell can re-compose them.
// `XiangQiKitDebugPanel` is the all-in-one assembly; the sub-pieces
// (`KitMiniBoard`, `KitAugmentationsGrid`, the tiles) work standalone
// off a `DebugProbe` snapshot.

export interface KitDebugPanelProps {
  probe: DebugProbe | null
  defaultOpen?: boolean
  className?: string
}

export function XiangQiKitDebugPanel({
  probe,
  defaultOpen,
  className,
}: KitDebugPanelProps) {
  return (
    <DebugPanel
      probe={probe}
      defaultOpen={defaultOpen}
      className={className}
    />
  )
}

export interface KitMiniBoardProps {
  board: Int8Array | ReadonlyArray<number>
  cellSize?: number
  caption?: string
  footer?: string
  className?: string
}

export function XiangQiKitMiniBoard(props: KitMiniBoardProps) {
  return <MiniBoard {...props} />
}

export interface KitAugmentationsGridProps {
  probe: DebugProbe
  cellSize?: number
  className?: string
}

export function XiangQiKitAugmentationsGrid({
  probe,
  cellSize,
  className,
}: KitAugmentationsGridProps) {
  return (
    <AugmentationsGrid
      variants={probe.variants}
      cellSize={cellSize}
      className={className}
    />
  )
}

export interface KitDebugTilesProps {
  probe: DebugProbe
  currentPlayer: Player
  className?: string
}

export function XiangQiKitDebugTiles({
  probe,
  currentPlayer,
  className,
}: KitDebugTilesProps) {
  return (
    <div
      className={
        className ?? 'grid gap-3 grid-cols-1 sm:grid-cols-3'
      }
    >
      <ActionCountTile
        count={probe.validActionCount}
        player={currentPlayer}
      />
      <SerializationProbeTile
        stateVectorLength={probe.stateVectorLength}
        stateRoundtripOk={probe.stateRoundtripOk}
      />
      <VariantCountTile count={probe.variants.length} />
    </div>
  )
}
