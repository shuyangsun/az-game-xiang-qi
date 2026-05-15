import React from 'react'
import type { Player } from '../../engine'

export interface GameStatusBarProps {
  currentPlayer: Player
  currentRound: number
  repeatCount: number
  inCheck: boolean
  isOver: boolean
  score: { red: number; black: number }
}

export function GameStatusBar({
  currentPlayer,
  currentRound,
  repeatCount,
  inCheck,
  isOver,
  score,
}: GameStatusBarProps) {
  const playerLabel = currentPlayer === 'red' ? '紅 (Red)' : '黑 (Black)'
  const playerColor =
    currentPlayer === 'red' ? 'text-red-600 dark:text-red-500' : 'text-slate-800 dark:text-slate-200'

  let statusMsg = `Side to move: ${playerLabel}`
  if (isOver) {
    if (score.red > score.black) statusMsg = 'Winner: 紅 (Red)'
    else if (score.black > score.red) statusMsg = 'Winner: 黑 (Black)'
    else statusMsg = 'Draw'
  }

  return (
    <div className="flex flex-wrap items-center justify-between p-4 bg-slate-50 dark:bg-slate-900 border-b border-slate-200 dark:border-slate-800 transition-colors">
      <div className="flex items-center gap-4">
        <div className={`text-lg font-bold ${isOver ? 'text-slate-900 dark:text-slate-100' : playerColor}`}>
          {statusMsg}
        </div>
        {inCheck && !isOver && (
          <div className="px-2 py-1 bg-red-100 dark:bg-red-900/40 text-red-700 dark:text-red-400 text-sm font-semibold rounded border border-red-200 dark:border-red-800">
            CHECK
          </div>
        )}
        {repeatCount > 1 && !isOver && (
          <div className="px-2 py-1 bg-amber-100 dark:bg-amber-900/40 text-amber-700 dark:text-amber-400 text-sm font-semibold rounded border border-amber-200 dark:border-amber-800">
            Position repeated {repeatCount}x
          </div>
        )}
      </div>
      <div className="text-slate-500 dark:text-slate-400 font-medium">Round: {currentRound}</div>
    </div>
  )
}
