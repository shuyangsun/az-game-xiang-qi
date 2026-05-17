import { useState } from 'react'
import { ChevronDown, ChevronUp } from 'lucide-react'
import { Button } from '#/components/ui/button'
import { cn } from '#/lib/utils'
import type { DebugProbe } from '../../engine'
import { AugmentationsGrid } from './AugmentationsGrid'

export interface DebugPanelProps {
  /**
   * Pre-computed probe snapshot. Pass `null` when the engine isn't
   * built with the debug binding so the panel can render a clear
   * remediation hint.
   */
  probe: DebugProbe | null
  defaultOpen?: boolean
  className?: string
}

/**
 * Collapsible debug surface showing the inference-time augmented
 * variants. The caller computes the `DebugProbe` once (keyed on
 * snapshot identity) so this panel and the sibling stat tiles
 * never disagree on what they're showing.
 */
export function DebugPanel({
  probe,
  defaultOpen = false,
  className,
}: DebugPanelProps) {
  const [open, setOpen] = useState(defaultOpen)
  const variantCount = probe?.variants.length ?? 0

  return (
    <div
      className={cn(
        'rounded-xl border bg-card text-card-foreground shadow-sm',
        className,
      )}
    >
      <button
        type="button"
        className="w-full flex items-center justify-between px-4 py-3 text-left hover:bg-accent/40 transition-colors rounded-t-xl"
        onClick={() => setOpen((v) => !v)}
        aria-expanded={open}
      >
        <div className="flex items-center gap-2">
          <span className="text-sm font-semibold">
            Augmented variants ({variantCount})
          </span>
          <span className="text-[11px] font-mono text-muted-foreground">
            inference-time data augmentation
          </span>
        </div>
        {open ? (
          <ChevronUp className="size-4 text-muted-foreground" />
        ) : (
          <ChevronDown className="size-4 text-muted-foreground" />
        )}
      </button>

      {open && (
        <div className="border-t px-4 py-4">
          {probe ? (
            <AugmentationsGrid variants={probe.variants} cellSize={18} />
          ) : (
            <DebugProbeMissingNotice />
          )}
        </div>
      )}
    </div>
  )
}

function DebugProbeMissingNotice() {
  return (
    <div className="flex items-start gap-3 rounded-md border border-amber-500/40 bg-amber-500/5 p-3 text-sm">
      <div className="flex flex-col gap-1.5">
        <span className="font-semibold text-amber-700 dark:text-amber-400">
          Debug probe unavailable
        </span>
        <span className="text-xs text-muted-foreground">
          The WASM module is missing <code>XqDebugProbeJs</code>. Rebuild
          it with <code>bun run wasm:build</code> from
          <code className="ml-1">gui/</code>.
        </span>
        <Button
          variant="outline"
          size="sm"
          className="mt-1 self-start"
          onClick={() => window.location.reload()}
        >
          Reload page
        </Button>
      </div>
    </div>
  )
}
