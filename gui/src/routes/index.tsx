import { createFileRoute } from '@tanstack/react-router';
import React, { useState, useMemo, useEffect, useRef } from 'react';
import { useXqGame, type Action, type Player } from '../engine';
import {
  XiangQiBoard,
  GameStatusBar,
  MoveList,
  ControlBar,
  DebugPanel,
  type Orientation,
} from '../components/xiangqi';

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

  // Update history logic
  const prevRoundRef = useRef(0)
  useEffect(() => {
    if (snapshot && engine) {
      if (snapshot.currentRound === 0) {
        setMoveHistory([])
      } else if (snapshot.currentRound > prevRoundRef.current) {
        // Appended an action
        if (snapshot.lastAction) {
          const actionStr = engine.actionToString(snapshot.lastAction)
          setMoveHistory((prev) => [...prev, actionStr])
        }
      } else if (snapshot.currentRound < prevRoundRef.current) {
        // Undo
        setMoveHistory((prev) => prev.slice(0, snapshot.currentRound))
      }
      prevRoundRef.current = snapshot.currentRound
    }
  }, [snapshot, engine])

  // Derived state for legal targets
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
      <div className="p-8 text-red-600">
        <h1 className="text-2xl font-bold">Failed to load engine</h1>
        <p>{error.message}</p>
      </div>
    )
  }

  if (!isReady || !snapshot || !engine) {
    return <div className="p-8">Loading engine...</div>
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
    <div className="min-h-screen bg-slate-100 dark:bg-slate-950 flex flex-col font-sans transition-colors">
      <GameStatusBar
        currentPlayer={snapshot.currentPlayer}
        currentRound={snapshot.currentRound}
        repeatCount={snapshot.repeatCount}
        inCheck={snapshot.inCheck}
        isOver={snapshot.isOver}
        score={snapshot.score}
      />

      <div className="flex-1 flex flex-col lg:flex-row overflow-hidden">
        <div className="flex-1 flex flex-col">
          <ControlBar
            orientation={orientation}
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
          <div className="flex-1 overflow-auto p-4 flex justify-center items-start">
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

        <div className="border-t lg:border-t-0 lg:border-l border-slate-200 dark:border-slate-800 bg-white dark:bg-slate-900 shadow-sm flex flex-col p-4 w-full lg:w-72 transition-colors">
          <MoveList moves={moveHistory} />
        </div>
      </div>

      {import.meta.env.VITE_XQ_DEBUG_PANEL && (
        <DebugPanel engine={engine} hoveredAction={null} />
      )}
    </div>
  )
}
