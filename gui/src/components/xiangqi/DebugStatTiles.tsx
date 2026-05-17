import { Badge } from '#/components/ui/badge'
import { cn } from '#/lib/utils'

export interface DebugStatTileProps {
  label: string
  value: React.ReactNode
  hint?: React.ReactNode
  className?: string
}

/**
 * Generic single-stat tile for the debug surface. The kit exports this
 * so consuming shells (alpha-zero training UI) can drop the same tile
 * shape into their own panels.
 */
export function DebugStatTile({
  label,
  value,
  hint,
  className,
}: DebugStatTileProps) {
  return (
    <div
      className={cn(
        'flex flex-col gap-1 rounded-md border bg-card/40 px-3 py-2',
        className,
      )}
    >
      <div className="text-[11px] font-mono uppercase tracking-wide text-muted-foreground">
        {label}
      </div>
      <div className="text-xl font-semibold tabular-nums leading-none">
        {value}
      </div>
      {hint && (
        <div className="text-[11px] font-mono text-muted-foreground">{hint}</div>
      )}
    </div>
  )
}

export interface ActionCountTileProps {
  count: number
  player: 'red' | 'black'
  className?: string
}

export function ActionCountTile({
  count,
  player,
  className,
}: ActionCountTileProps) {
  return (
    <DebugStatTile
      label="Legal moves"
      value={count}
      hint={
        <span className="flex items-center gap-1.5">
          <span
            className="size-2 rounded-full"
            style={{
              backgroundColor:
                player === 'red' ? 'var(--xq-red)' : 'var(--xq-black)',
            }}
            aria-hidden
          />
          for {player === 'red' ? 'red' : 'black'} to move
        </span>
      }
      className={className}
    />
  )
}

export interface SerializationProbeTileProps {
  stateVectorLength: number
  stateRoundtripOk: boolean
  className?: string
}

export function SerializationProbeTile({
  stateVectorLength,
  stateRoundtripOk,
  className,
}: SerializationProbeTileProps) {
  return (
    <DebugStatTile
      label="State serialization"
      value={
        <span className="flex items-baseline gap-2">
          <span>{stateVectorLength}</span>
          <span className="text-xs font-mono text-muted-foreground">
            floats
          </span>
        </span>
      }
      hint={
        <span className="flex items-center gap-1.5">
          <Badge
            variant={stateRoundtripOk ? 'success' : 'destructive'}
            className="px-1.5 py-0 text-[10px]"
          >
            {stateRoundtripOk ? 'roundtrip ok' : 'roundtrip mismatch'}
          </Badge>
          <span>decode → canonical board</span>
        </span>
      }
      className={className}
    />
  )
}

export interface VariantCountTileProps {
  count: number
  className?: string
}

export function VariantCountTile({ count, className }: VariantCountTileProps) {
  return (
    <DebugStatTile
      label="Augmented variants"
      value={count}
      hint={
        <span>
          inference-time data augmentation (original + horizontal mirror)
        </span>
      }
      className={className}
    />
  )
}
