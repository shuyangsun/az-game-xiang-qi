import { Plus, RotateCw, Undo2 } from 'lucide-react'
import { useEffect, useRef, useState } from 'react'
import type { Player } from '../../engine'
import { Button } from '#/components/ui/button'
import { cn } from '#/lib/utils'

export interface ControlBarProps {
  onToggleOrientation: () => void
  canUndo: boolean
  onUndo: () => void
  onNewGame: (startingPlayer: Player) => void
  className?: string
}

export function ControlBar({
  onToggleOrientation,
  canUndo,
  onUndo,
  onNewGame,
  className,
}: ControlBarProps) {
  const [showNewGameMenu, setShowNewGameMenu] = useState(false)
  const popoverRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (!showNewGameMenu) return
    const onClick = (e: MouseEvent) => {
      if (!popoverRef.current?.contains(e.target as Node)) {
        setShowNewGameMenu(false)
      }
    }
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') setShowNewGameMenu(false)
    }
    window.addEventListener('mousedown', onClick)
    window.addEventListener('keydown', onKey)
    return () => {
      window.removeEventListener('mousedown', onClick)
      window.removeEventListener('keydown', onKey)
    }
  }, [showNewGameMenu])

  return (
    <div
      className={cn(
        'flex items-center gap-2 px-4 py-3 border-b bg-card/40',
        className,
      )}
    >
      <div ref={popoverRef} className="relative">
        <Button
          variant="default"
          size="sm"
          onClick={() => setShowNewGameMenu((v) => !v)}
          aria-expanded={showNewGameMenu}
          aria-haspopup="menu"
        >
          <Plus />
          New Game
        </Button>
        {showNewGameMenu && (
          <div
            role="menu"
            className="absolute top-full left-0 mt-1.5 min-w-44 rounded-md border bg-popover text-popover-foreground shadow-md z-10 p-1"
          >
            <button
              type="button"
              role="menuitem"
              className="w-full flex items-center gap-2 rounded-sm px-3 py-2 text-sm hover:bg-accent hover:text-accent-foreground transition-colors"
              onClick={() => {
                onNewGame('red')
                setShowNewGameMenu(false)
              }}
            >
              <span
                className="size-2.5 rounded-full"
                style={{ backgroundColor: 'var(--xq-red)' }}
                aria-hidden
              />
              Red starts
            </button>
            <button
              type="button"
              role="menuitem"
              className="w-full flex items-center gap-2 rounded-sm px-3 py-2 text-sm hover:bg-accent hover:text-accent-foreground transition-colors"
              onClick={() => {
                onNewGame('black')
                setShowNewGameMenu(false)
              }}
            >
              <span
                className="size-2.5 rounded-full"
                style={{ backgroundColor: 'var(--xq-black)' }}
                aria-hidden
              />
              Black starts
            </button>
          </div>
        )}
      </div>

      <Button
        variant="outline"
        size="sm"
        disabled={!canUndo}
        onClick={onUndo}
      >
        <Undo2 />
        Undo
      </Button>

      <Button
        variant="ghost"
        size="sm"
        className="ml-auto"
        onClick={onToggleOrientation}
      >
        <RotateCw />
        Flip board
      </Button>
    </div>
  )
}
