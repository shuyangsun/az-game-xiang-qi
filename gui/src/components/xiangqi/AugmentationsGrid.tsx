import { Badge } from '#/components/ui/badge'
import { cn } from '#/lib/utils'
import type { DebugProbeVariant } from '../../engine'
import { MiniBoard } from './MiniBoard'

export interface AugmentationsGridProps {
  variants: ReadonlyArray<DebugProbeVariant>
  cellSize?: number
  className?: string
}

/**
 * Grid of mini boards, one per inference-time augmented variant.
 * Each card carries the variant's name, side to move, legal-move
 * count, and a roundtrip indicator so a glance reveals which
 * variant — if any — fails the serializer/board self-check.
 *
 * Pure presentational; the kit's debug panel composes it but the
 * shell can also drop it into its own debug surfaces.
 */
export function AugmentationsGrid({
  variants,
  cellSize = 22,
  className,
}: AugmentationsGridProps) {
  return (
    <div
      className={cn(
        'grid gap-3 grid-cols-1 sm:grid-cols-2 lg:grid-cols-3',
        className,
      )}
    >
      {variants.map((v) => (
        <div
          key={v.index}
          className="flex flex-col gap-2 rounded-md border bg-card/40 p-3"
        >
          <div className="flex items-center justify-between gap-2">
            <div className="flex items-center gap-2">
              <span className="text-sm font-semibold">{v.name}</span>
              <span className="text-[11px] font-mono text-muted-foreground">
                #{v.index}
              </span>
            </div>
            <Badge
              variant={v.stateRoundtripOk ? 'success' : 'destructive'}
              className="px-1.5 py-0 text-[10px]"
            >
              {v.stateRoundtripOk ? 'ok' : 'fail'}
            </Badge>
          </div>
          <MiniBoard
            board={v.board}
            cellSize={cellSize}
            orientation="red-bottom"
          />
          <div className="flex items-center justify-between text-[11px] font-mono text-muted-foreground">
            <span className="flex items-center gap-1.5">
              <span
                className="size-2 rounded-full"
                style={{
                  backgroundColor:
                    v.currentPlayer === 'red'
                      ? 'var(--xq-red)'
                      : 'var(--xq-black)',
                }}
                aria-hidden
              />
              {v.currentPlayer} to move
            </span>
            <span>{v.validActionCount} moves</span>
          </div>
          <div className="text-[11px] font-mono text-muted-foreground">
            state: {v.stateVectorLength} floats
          </div>
        </div>
      ))}
    </div>
  )
}
