import { createFileRoute } from '@tanstack/react-router'
import { useEffect, useMemo, useRef, useState } from 'react'
import { Swords } from 'lucide-react'
import { useXqGame } from '../engine'
import type { Player } from '../engine'
import {
  ControlBar,
  DebugPanel,
  GameStatusBar,
  MoveList,
  XiangQiBoard,
} from '../components/xiangqi'
import type { Orientation } from '../components/xiangqi'
import { ThemeToggle } from '../components/ThemeToggle'

export const Route = createFileRoute('/')({ component: Home })

function Home() {
  const {
    engine,
    snapshot,
    error,
    applyAction,
    undoLastAction,
    reset,
    isReady,
  } = useXqGame()

  const [orientation, setOrientation] = useState<Orientation>('red-bottom')
  const [selectedCell, setSelectedCell] = useState<number | null>(null)
  const [moveHistory, setMoveHistory] = useState<string[]>([])

  const prevRoundRef = useRef(0)
  useEffect(() => {
    if (snapshot && engine) {
      if (snapshot.currentRound === 0) {
        setMoveHistory([])
      } else if (snapshot.currentRound > prevRoundRef.current) {
        if (snapshot.lastAction) {
          const actionStr = engine.actionToString(snapshot.lastAction)
          setMoveHistory((prev) => [...prev, actionStr])
        }
      } else if (snapshot.currentRound < prevRoundRef.current) {
        setMoveHistory((prev) => prev.slice(0, snapshot.currentRound))
      }
      prevRoundRef.current = snapshot.currentRound
    }
  }, [snapshot, engine])

  const legalTargets = useMemo(() => {
    const targets = new Set<number>()
    if (snapshot && selectedCell !== null) {
      for (const action of snapshot.validActions) {
        if (action.from === selectedCell) {
          targets.add(action.to)
        }
      }
    }
    return targets
  }, [snapshot, selectedCell])

  if (error) {
    return (
      <div className="min-h-screen grid place-items-center p-8">
        <div className="rounded-xl border border-destructive/40 bg-destructive/5 px-6 py-5 max-w-md text-center">
          <h1 className="text-lg font-semibold text-destructive">
            Failed to load engine
          </h1>
          <p className="mt-1 text-sm text-muted-foreground">{error.message}</p>
        </div>
      </div>
    )
  }

  if (!isReady || !snapshot || !engine) {
    return (
      <div className="min-h-screen grid place-items-center p-8 text-muted-foreground">
        Loading engine…
      </div>
    )
  }

  const handleCellClick = (cellIdx: number) => {
    if (snapshot.isOver) return

    if (legalTargets.has(cellIdx) && selectedCell !== null) {
      applyAction({ from: selectedCell, to: cellIdx })
      setSelectedCell(null)
    } else {
      const val = snapshot.board[cellIdx]
      const isRedPiece = val > 0
      const isBlackPiece = val < 0
      const ownsPiece =
        (snapshot.currentPlayer === 'red' && isRedPiece) ||
        (snapshot.currentPlayer === 'black' && isBlackPiece)

      if (ownsPiece) {
        setSelectedCell(cellIdx)
      } else {
        setSelectedCell(null)
      }
    }
  }

  const handleToggleOrientation = () => {
    setOrientation((prev) =>
      prev === 'red-bottom' ? 'black-bottom' : 'red-bottom',
    )
  }

  const findGeneralCell = (player: Player): number | null => {
    const targetVal = player === 'red' ? 1 : -1
    for (let i = 0; i < 90; i++) {
      if (snapshot.board[i] === targetVal) return i
    }
    return null
  }

  let inCheckCell: number | null = null
  if (snapshot.inCheck) {
    inCheckCell = findGeneralCell(snapshot.currentPlayer)
  }

  return (
    <div className="min-h-screen flex flex-col bg-background text-foreground font-sans">
      <header className="sticky top-0 z-40 border-b bg-background/80 backdrop-blur supports-[backdrop-filter]:bg-background/60">
        <div className="max-w-[1400px] mx-auto px-4 h-14 flex items-center gap-3">
          <div className="flex items-center gap-2">
            <div
              className="size-7 rounded-md grid place-items-center text-white shadow-sm"
              style={{
                background:
                  'linear-gradient(135deg, var(--xq-red) 0%, oklch(0.45 0.2 28) 100%)',
              }}
            >
              <Swords className="size-4" strokeWidth={2.5} />
            </div>
            <div className="flex flex-col leading-tight">
              <span className="text-sm font-semibold tracking-tight">
                Xiang Qi
              </span>
              <span className="text-[10px] font-mono text-muted-foreground -mt-0.5">
                象棋 · Chinese chess
              </span>
            </div>
          </div>
          <div className="ml-auto flex items-center gap-2">
            <ThemeToggle />
          </div>
        </div>
      </header>

      <main className="flex-1">
        <div className="max-w-[1400px] mx-auto px-4 py-6 flex flex-col lg:flex-row gap-6">
          <section className="flex-1 min-w-0">
            <div className="rounded-xl border bg-card text-card-foreground shadow-sm overflow-hidden">
              <GameStatusBar
                currentPlayer={snapshot.currentPlayer}
                currentRound={snapshot.currentRound}
                repeatCount={snapshot.repeatCount}
                inCheck={snapshot.inCheck}
                isOver={snapshot.isOver}
                score={snapshot.score}
              />
              <ControlBar
                onToggleOrientation={handleToggleOrientation}
                canUndo={snapshot.currentRound > 0}
                onUndo={() => {
                  undoLastAction()
                  setSelectedCell(null)
                }}
                onNewGame={(sp) => {
                  reset(sp)
                  setSelectedCell(null)
                }}
              />
              <div className="p-4 sm:p-6 flex justify-center items-start">
                <XiangQiBoard
                  board={snapshot.board}
                  orientation={orientation}
                  selectedCell={selectedCell}
                  legalTargets={legalTargets}
                  lastMove={snapshot.lastAction}
                  inCheckCell={inCheckCell}
                  onCellClick={handleCellClick}
                />
              </div>
            </div>
          </section>

          <aside className="w-full lg:w-80 lg:shrink-0">
            <div className="h-[calc(100vh-8rem)] lg:h-[calc(100vh-9rem)] sticky top-20">
              <MoveList moves={moveHistory} />
            </div>
          </aside>
        </div>
      </main>

      {import.meta.env.VITE_XQ_DEBUG_PANEL && (
        <DebugPanel engine={engine} hoveredAction={null} />
      )}
    </div>
  )
}
