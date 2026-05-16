import type { Player } from '../../engine'
import { Badge } from '#/components/ui/badge'
import { cn } from '#/lib/utils'

export interface GameStatusBarProps {
  currentPlayer: Player
  currentRound: number
  repeatCount: number
  inCheck: boolean
  isOver: boolean
  score: { red: number; black: number }
  className?: string
}

export function GameStatusBar({
  currentPlayer,
  currentRound,
  repeatCount,
  inCheck,
  isOver,
  score,
  className,
}: GameStatusBarProps) {
  let statusLabel: string
  let statusKind: 'turn' | 'over'
  if (isOver) {
    statusKind = 'over'
    if (score.red > score.black) statusLabel = 'Red wins'
    else if (score.black > score.red) statusLabel = 'Black wins'
    else statusLabel = 'Draw'
  } else {
    statusKind = 'turn'
    statusLabel = currentPlayer === 'red' ? '紅 Red to move' : '黑 Black to move'
  }

  const turnDotColor =
    currentPlayer === 'red' ? 'var(--xq-red)' : 'var(--xq-black)'

  return (
    <div
      className={cn(
        'flex flex-wrap items-center justify-between gap-3 px-4 py-3 border-b bg-card/40',
        className,
      )}
    >
      <div className="flex items-center gap-3">
        {statusKind === 'turn' ? (
          <div className="flex items-center gap-2">
            <span
              className="size-3 rounded-full ring-1 ring-black/20 dark:ring-white/15 shadow-xs"
              style={{ backgroundColor: turnDotColor }}
              aria-hidden
            />
            <span className="text-base font-semibold">{statusLabel}</span>
          </div>
        ) : (
          <span className="text-base font-semibold">{statusLabel}</span>
        )}
        {inCheck && !isOver && (
          <Badge variant="destructive" className="uppercase tracking-wide">
            Check
          </Badge>
        )}
        {repeatCount > 1 && !isOver && (
          <Badge variant="warning">Repeated ×{repeatCount}</Badge>
        )}
      </div>
      <div className="text-sm text-muted-foreground font-mono">
        Round {currentRound}
      </div>
    </div>
  )
}
