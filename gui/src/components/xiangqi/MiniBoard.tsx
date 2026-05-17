import { cn } from '#/lib/utils'

const RED_CHARS = ['', '帥', '仕', '相', '馬', '車', '砲', '兵']
const BLACK_CHARS = ['', '將', '士', '象', '傌', '俥', '炮', '卒']

export type MiniBoardOrientation = 'red-bottom' | 'black-bottom'

export interface MiniBoardProps {
  /** Length-90 row-major board: rows 0..9, cols 0..8, cells -7..7. */
  board: Int8Array | ReadonlyArray<number>
  cellSize?: number
  orientation?: MiniBoardOrientation
  /** Title shown in the corner ribbon. */
  caption?: string
  /** Footer text under the board. */
  footer?: string
  className?: string
}

/**
 * Read-only, low-cost board renderer for the debug panel.
 *
 * Uses a single inline-SVG render path (no interactivity, no
 * gradients, no shadows) so a grid of these can show many augmented
 * boards side by side without a noticeable hit. The shell can drop
 * it into its own debug surfaces by importing it from the kit.
 */
export function MiniBoard({
  board,
  cellSize = 22,
  orientation = 'red-bottom',
  caption,
  footer,
  className,
}: MiniBoardProps) {
  const padding = Math.round(cellSize * 0.6)
  const width = cellSize * 8 + padding * 2
  const height = cellSize * 9 + padding * 2
  const isRedBottom = orientation === 'red-bottom'

  const getCoords = (row: number, col: number) => {
    const visualRow = isRedBottom ? 9 - row : row
    const visualCol = isRedBottom ? col : 8 - col
    return {
      x: padding + visualCol * cellSize,
      y: padding + visualRow * cellSize,
    }
  }

  const riverUpperY = padding + 4 * cellSize
  const riverLowerY = padding + 5 * cellSize
  const topY = padding
  const bottomY = padding + 9 * cellSize

  const lines: Array<React.ReactNode> = []

  for (let c = 0; c < 9; c++) {
    const x = padding + (isRedBottom ? c : 8 - c) * cellSize
    if (c === 0 || c === 8) {
      lines.push(
        <line
          key={`v${c}`}
          x1={x}
          y1={topY}
          x2={x}
          y2={bottomY}
          stroke="var(--xq-board-ink)"
          strokeWidth="0.8"
        />,
      )
    } else {
      lines.push(
        <line
          key={`vt${c}`}
          x1={x}
          y1={topY}
          x2={x}
          y2={riverUpperY}
          stroke="var(--xq-board-ink)"
          strokeWidth="0.8"
        />,
        <line
          key={`vb${c}`}
          x1={x}
          y1={riverLowerY}
          x2={x}
          y2={bottomY}
          stroke="var(--xq-board-ink)"
          strokeWidth="0.8"
        />,
      )
    }
  }
  for (let r = 0; r < 10; r++) {
    const left = getCoords(r, 0)
    const right = getCoords(r, 8)
    lines.push(
      <line
        key={`h${r}`}
        x1={left.x}
        y1={left.y}
        x2={right.x}
        y2={right.y}
        stroke="var(--xq-board-ink)"
        strokeWidth="0.8"
      />,
    )
  }

  const drawPalace = (rStart: number) => {
    const tl = getCoords(rStart, 3)
    const tr = getCoords(rStart, 5)
    const bl = getCoords(rStart + 2, 3)
    const br = getCoords(rStart + 2, 5)
    return (
      <g key={`palace-${rStart}`}>
        <line
          x1={tl.x}
          y1={tl.y}
          x2={br.x}
          y2={br.y}
          stroke="var(--xq-board-ink)"
          strokeWidth="0.8"
        />
        <line
          x1={tr.x}
          y1={tr.y}
          x2={bl.x}
          y2={bl.y}
          stroke="var(--xq-board-ink)"
          strokeWidth="0.8"
        />
      </g>
    )
  }

  const pieces: Array<React.ReactNode> = []
  const pieceR = cellSize * 0.42
  for (let row = 0; row < 10; row++) {
    for (let col = 0; col < 9; col++) {
      const val = board[row * 9 + col] ?? 0
      if (val === 0) continue
      const { x, y } = getCoords(row, col)
      const isRed = val > 0
      const absVal = Math.abs(val)
      const char = isRed ? RED_CHARS[absVal] : BLACK_CHARS[absVal]
      const tint = isRed ? 'var(--xq-red)' : 'var(--xq-black)'
      pieces.push(
        <g key={`p-${row}-${col}`}>
          <circle
            cx={x}
            cy={y}
            r={pieceR}
            fill="var(--xq-piece-face)"
            stroke={tint}
            strokeWidth="0.9"
          />
          <text
            x={x}
            y={y}
            textAnchor="middle"
            dominantBaseline="central"
            fontSize={cellSize * 0.55}
            fontWeight={700}
            fill={tint}
            style={{
              fontFamily:
                "ui-serif, 'Songti SC', 'Noto Serif CJK SC', 'Source Han Serif SC', serif",
            }}
          >
            {char}
          </text>
        </g>,
      )
    }
  }

  return (
    <div className={cn('flex flex-col gap-1', className)}>
      {caption && (
        <div className="text-[11px] font-mono text-muted-foreground uppercase tracking-wide">
          {caption}
        </div>
      )}
      <svg
        viewBox={`0 0 ${width} ${height}`}
        width={width}
        height={height}
        className="rounded-md ring-1 ring-border bg-[var(--xq-board-surface)]"
      >
        {lines}
        {drawPalace(0)}
        {drawPalace(7)}
        {pieces}
      </svg>
      {footer && (
        <div className="text-[11px] font-mono text-muted-foreground">
          {footer}
        </div>
      )}
    </div>
  )
}
