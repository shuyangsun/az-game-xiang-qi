import { useEffect, useRef } from 'react'
import { cn } from '#/lib/utils'

export interface MoveListProps {
  moves: string[]
  className?: string
}

export function MoveList({ moves, className }: MoveListProps) {
  const scrollRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight
    }
  }, [moves])

  return (
    <div
      className={cn(
        'flex flex-col rounded-xl border bg-card text-card-foreground shadow-sm w-full h-full overflow-hidden',
        className,
      )}
    >
      <div className="border-b px-4 py-2.5 text-sm font-semibold text-card-foreground bg-card/40">
        Move history
      </div>
      <div ref={scrollRef} className="flex-1 overflow-y-auto p-2">
        {moves.length === 0 ? (
          <div className="px-3 py-6 text-center text-sm text-muted-foreground italic">
            No moves yet
          </div>
        ) : (
          <ol className="space-y-0.5">
            {moves.map((move, i) => (
              <li
                key={i}
                className="flex items-baseline gap-3 rounded-sm px-3 py-1.5 text-sm font-mono hover:bg-accent/60 transition-colors"
              >
                <span className="w-6 shrink-0 text-right text-xs tabular-nums text-muted-foreground">
                  {i + 1}.
                </span>
                <span className="text-foreground">{move}</span>
              </li>
            ))}
          </ol>
        )}
      </div>
    </div>
  )
}
