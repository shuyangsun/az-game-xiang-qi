import { useState } from 'react'
import type { Player } from '../../engine'

export interface ControlBarProps {
  onToggleOrientation: () => void
  canUndo: boolean
  onUndo: () => void
  onNewGame: (startingPlayer: Player) => void
}

export function ControlBar({
  onToggleOrientation,
  canUndo,
  onUndo,
  onNewGame,
}: ControlBarProps) {
  const [showNewGameMenu, setShowNewGameMenu] = useState(false)

  return (
    <div className="flex items-center gap-4 p-4 bg-white dark:bg-slate-900 border-b border-slate-200 dark:border-slate-800 transition-colors">
      <div className="relative">
        <button
          className="px-4 py-2 bg-blue-600 hover:bg-blue-700 dark:bg-blue-700 dark:hover:bg-blue-600 text-white font-semibold rounded shadow-sm transition-colors"
          onClick={() => setShowNewGameMenu(!showNewGameMenu)}
        >
          New Game
        </button>
        {showNewGameMenu && (
          <div className="absolute top-full left-0 mt-1 bg-white dark:bg-slate-800 border border-slate-200 dark:border-slate-700 rounded shadow-lg z-10 py-1 w-40">
            <button
              className="w-full text-left px-4 py-2 hover:bg-slate-50 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 transition-colors"
              onClick={() => {
                onNewGame('red')
                setShowNewGameMenu(false)
              }}
            >
              Red Starts
            </button>
            <button
              className="w-full text-left px-4 py-2 hover:bg-slate-50 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 transition-colors"
              onClick={() => {
                onNewGame('black')
                setShowNewGameMenu(false)
              }}
            >
              Black Starts
            </button>
          </div>
        )}
      </div>

      <button
        className="px-4 py-2 bg-slate-200 hover:bg-slate-300 dark:bg-slate-800 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 font-semibold rounded shadow-sm transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
        disabled={!canUndo}
        onClick={onUndo}
      >
        Undo
      </button>

      <button
        className="px-4 py-2 bg-slate-200 hover:bg-slate-300 dark:bg-slate-800 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 font-semibold rounded shadow-sm transition-colors ml-auto"
        onClick={onToggleOrientation}
      >
        Flip Board
      </button>
    </div>
  )
}
