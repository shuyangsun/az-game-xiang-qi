import { useEffect, useRef, useState } from 'react'
import type { Action } from '../../engine'

export type Orientation = 'red-bottom' | 'black-bottom'

export interface XiangQiBoardProps {
  board: Int8Array
  orientation: Orientation
  selectedCell: number | null
  legalTargets: ReadonlySet<number>
  lastMove: Action | null
  inCheckCell: number | null
  onCellClick: (cell: number) => void
  className?: string
}

const RED_CHARS = ['', '帥', '仕', '相', '馬', '車', '砲', '兵']
const BLACK_CHARS = ['', '將', '士', '象', '傌', '俥', '炮', '卒']

const DEFAULT_CELL_SIZE = 56
const CELL_SIZE_VAR = '--xq-board-cell-size'

// Authentic Xiangqi position marks — pawn launch points and the four
// cannon stations. Drawn as small corner brackets at each listed
// intersection; bracket count drops to two on the file edges so the
// marks never poke past the board.
const POSITION_MARKERS: ReadonlyArray<readonly [row: number, col: number]> = [
  // red cannons
  [2, 1],
  [2, 7],
  // red pawns
  [3, 0],
  [3, 2],
  [3, 4],
  [3, 6],
  [3, 8],
  // black pawns
  [6, 0],
  [6, 2],
  [6, 4],
  [6, 6],
  [6, 8],
  // black cannons
  [7, 1],
  [7, 7],
]

function readCellSize(el: Element | null): number {
  if (!el) return DEFAULT_CELL_SIZE
  const raw = getComputedStyle(el).getPropertyValue(CELL_SIZE_VAR).trim()
  if (!raw) return DEFAULT_CELL_SIZE
  const parsed = parseFloat(raw)
  return Number.isFinite(parsed) && parsed > 0 ? parsed : DEFAULT_CELL_SIZE
}

export function XiangQiBoard({
  board,
  orientation,
  selectedCell,
  legalTargets,
  lastMove,
  inCheckCell,
  onCellClick,
  className,
}: XiangQiBoardProps) {
  const containerRef = useRef<HTMLDivElement>(null)
  const [cellSize, setCellSize] = useState<number>(DEFAULT_CELL_SIZE)

  useEffect(() => {
    setCellSize(readCellSize(containerRef.current))
  }, [])

  const padding = Math.round(cellSize * 0.6)
  const width = cellSize * 8 + padding * 2
  const height = cellSize * 9 + padding * 2

  const isRedBottom = orientation === 'red-bottom'

  // Maps logical (row, col) — row 0 = red's home rank — to SVG (x, y).
  // Orientation rotates the board 180° (both axes), but the river always
  // sits visually between rows 4 and 5 in SVG-space, so the per-file
  // line splits use direct y-coords below rather than going through this
  // helper a second time (that mapping is what made the flipped board
  // paint inner files straight through the river).
  const getCoords = (row: number, col: number) => {
    const visualRow = isRedBottom ? 9 - row : row
    const visualCol = isRedBottom ? col : 8 - col
    return {
      x: padding + visualCol * cellSize,
      y: padding + visualRow * cellSize,
    }
  }

  const getCellIdx = (row: number, col: number) => row * 9 + col

  // River always sits between visual rows 4 and 5 in SVG-y. Computing
  // the gap from raw SVG coordinates — not from the orientation-flipped
  // logical-row mapping — keeps inner files from crossing the river
  // when the board is flipped.
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
          strokeWidth="1.2"
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
          strokeWidth="1.2"
        />,
      )
      lines.push(
        <line
          key={`vb${c}`}
          x1={x}
          y1={riverLowerY}
          x2={x}
          y2={bottomY}
          stroke="var(--xq-board-ink)"
          strokeWidth="1.2"
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
        strokeWidth="1.2"
      />,
    )
  }

  // Palace diagonals (×) at both 3×3 corners.
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
          strokeWidth="1.2"
        />
        <line
          x1={tr.x}
          y1={tr.y}
          x2={bl.x}
          y2={bl.y}
          stroke="var(--xq-board-ink)"
          strokeWidth="1.2"
        />
      </g>
    )
  }

  // Pawn/cannon corner brackets.
  const positionMarks: Array<React.ReactNode> = []
  const markOffset = cellSize * 0.1
  const markArm = cellSize * 0.16
  for (const [row, col] of POSITION_MARKERS) {
    const { x, y } = getCoords(row, col)
    // Edge detection is in *visual* x — under a flipped board, logical
    // col 0 lands on the visual right edge.
    const visualCol = isRedBottom ? col : 8 - col
    const isLeftEdge = visualCol === 0
    const isRightEdge = visualCol === 8
    const quadrants: ReadonlyArray<[number, number, boolean]> = [
      [-1, -1, !isLeftEdge],
      [1, -1, !isRightEdge],
      [-1, 1, !isLeftEdge],
      [1, 1, !isRightEdge],
    ]
    for (let q = 0; q < quadrants.length; q++) {
      const [sx, sy, show] = quadrants[q]
      if (!show) continue
      positionMarks.push(
        <path
          key={`m-${row}-${col}-${q}`}
          d={`M ${x + sx * (markOffset + markArm)} ${y + sy * markOffset} L ${x + sx * markOffset} ${y + sy * markOffset} L ${x + sx * markOffset} ${y + sy * (markOffset + markArm)}`}
          stroke="var(--xq-board-ink-muted)"
          strokeWidth="1.1"
          fill="none"
          strokeLinecap="round"
        />,
      )
    }
  }

  const pieces: Array<React.ReactNode> = []
  const overlays: Array<React.ReactNode> = []
  const hitboxes: Array<React.ReactNode> = []

  for (let row = 0; row < 10; row++) {
    for (let col = 0; col < 9; col++) {
      const idx = getCellIdx(row, col)
      const val = board[idx]
      const { x, y } = getCoords(row, col)

      hitboxes.push(
        <rect
          key={`hitbox-${idx}`}
          x={x - cellSize / 2}
          y={y - cellSize / 2}
          width={cellSize}
          height={cellSize}
          fill="transparent"
          onClick={() => onCellClick(idx)}
          style={{ cursor: 'pointer' }}
        />,
      )

      // Last-move marker: soft tint + L-shaped corner brackets in the
      // theme's `--xq-last-move` accent. Replaces the prior yellow box.
      if (lastMove && (lastMove.from === idx || lastMove.to === idx)) {
        const half = cellSize / 2
        const cornerInset = cellSize * 0.08
        const cornerArm = cellSize * 0.22
        const cornerKeys: ReadonlyArray<[number, number]> = [
          [-1, -1],
          [1, -1],
          [-1, 1],
          [1, 1],
        ]
        overlays.push(
          <g key={`last-${idx}`} pointerEvents="none">
            <rect
              x={x - half + cornerInset}
              y={y - half + cornerInset}
              width={cellSize - cornerInset * 2}
              height={cellSize - cornerInset * 2}
              rx={cellSize * 0.08}
              fill="var(--xq-last-move)"
              opacity={0.12}
            />
            {cornerKeys.map(([sx, sy]) => {
              const ax = x + sx * (half - cornerInset)
              const ay = y + sy * (half - cornerInset)
              return (
                <path
                  key={`lc-${sx}-${sy}`}
                  d={`M ${ax - sx * cornerArm} ${ay} L ${ax} ${ay} L ${ax} ${ay - sy * cornerArm}`}
                  stroke="var(--xq-last-move)"
                  strokeWidth="2"
                  fill="none"
                  strokeLinecap="round"
                  opacity={0.9}
                />
              )
            })}
          </g>,
        )
      }

      if (selectedCell === idx) {
        overlays.push(
          <circle
            key={`sel-${idx}`}
            cx={x}
            cy={y}
            r={cellSize * 0.46}
            fill="none"
            stroke="var(--xq-last-move)"
            strokeWidth="3"
            opacity={0.9}
            pointerEvents="none"
          />,
        )
      }

      if (legalTargets.has(idx)) {
        if (val === 0) {
          overlays.push(
            <circle
              key={`tgt-${idx}`}
              cx={x}
              cy={y}
              r={cellSize * 0.14}
              className="fill-emerald-600 dark:fill-emerald-400"
              opacity={0.7}
              pointerEvents="none"
            />,
          )
        } else {
          overlays.push(
            <circle
              key={`tgt-${idx}`}
              cx={x}
              cy={y}
              r={cellSize * 0.46}
              fill="none"
              className="stroke-emerald-600 dark:stroke-emerald-400"
              strokeWidth="3"
              opacity={0.85}
              pointerEvents="none"
            />,
          )
        }
      }

      if (val !== 0) {
        const isRed = val > 0
        const absVal = Math.abs(val)
        const char = isRed ? RED_CHARS[absVal] : BLACK_CHARS[absVal]
        const isCheck = inCheckCell === idx
        const sideStroke = isRed ? 'var(--xq-red)' : 'var(--xq-black)'
        const sideFill = isRed ? 'var(--xq-red)' : 'var(--xq-black)'
        const pieceR = cellSize * 0.4
        const innerR = cellSize * 0.32

        pieces.push(
          <g key={`piece-${idx}`} style={{ pointerEvents: 'none' }}>
            {/* Soft shadow disc — cheap drop-shadow alternative. */}
            <circle
              cx={x}
              cy={y + 1.5}
              r={pieceR}
              fill="rgb(0 0 0 / 0.22)"
            />
            {/* Disc face with the shared piece gradient. */}
            <circle
              cx={x}
              cy={y}
              r={pieceR}
              fill="url(#xq-piece-face)"
            />
            {/* Side-coloured outer ring. */}
            <circle
              cx={x}
              cy={y}
              r={pieceR - 1}
              fill="none"
              stroke={sideStroke}
              strokeWidth="2.2"
            />
            {/* Inner decorative ring. */}
            <circle
              cx={x}
              cy={y}
              r={innerR}
              fill="none"
              stroke={sideStroke}
              strokeWidth="0.8"
              opacity={0.55}
            />
            {/* Character glyph. */}
            <text
              x={x}
              y={y}
              textAnchor="middle"
              dominantBaseline="central"
              fontSize={cellSize * 0.52}
              fontWeight={700}
              fill={sideFill}
              style={{
                fontFamily:
                  "ui-serif, 'Songti SC', 'Noto Serif CJK SC', 'Source Han Serif SC', serif",
              }}
            >
              {char}
            </text>
            {isCheck && (
              <circle
                cx={x}
                cy={y}
                r={pieceR + 2}
                fill="none"
                stroke="var(--destructive)"
                strokeWidth="3"
                className="animate-pulse"
                opacity={0.9}
              />
            )}
          </g>,
        )
      }
    }
  }

  // File / rank labels — quiet, lowered contrast.
  const labels: Array<React.ReactNode> = []
  for (let c = 0; c < 9; c++) {
    const x = padding + (isRedBottom ? c : 8 - c) * cellSize
    labels.push(
      <text
        key={`l-col-${c}`}
        x={x}
        y={height - padding * 0.35}
        textAnchor="middle"
        fontSize={cellSize * 0.22}
        fill="var(--xq-board-ink-muted)"
        style={{ fontFeatureSettings: '"tnum"' }}
      >
        {String.fromCharCode(97 + c)}
      </text>,
    )
  }
  for (let r = 0; r < 10; r++) {
    const y = padding + (isRedBottom ? 9 - r : r) * cellSize
    labels.push(
      <text
        key={`l-row-${r}`}
        x={padding * 0.4}
        y={y}
        dominantBaseline="central"
        textAnchor="middle"
        fontSize={cellSize * 0.22}
        fill="var(--xq-board-ink-muted)"
        style={{ fontFeatureSettings: '"tnum"' }}
      >
        {r}
      </text>,
    )
  }

  return (
    <div
      ref={containerRef}
      className={className ?? 'flex justify-center select-none'}
    >
      <svg
        viewBox={`0 0 ${width} ${height}`}
        width={width}
        height={height}
        className="rounded-xl shadow-md ring-1 ring-black/10 dark:ring-white/10"
        style={{ maxWidth: '100%', height: 'auto' }}
      >
        <defs>
          <radialGradient
            id="xq-piece-face"
            cx="42%"
            cy="34%"
            r="0.78"
          >
            <stop offset="0%" stopColor="var(--xq-piece-face)" />
            <stop offset="100%" stopColor="var(--xq-piece-face-edge)" />
          </radialGradient>
          <linearGradient id="xq-board-surface" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stopColor="var(--xq-board-surface)" />
            <stop offset="100%" stopColor="var(--xq-board-surface-edge)" />
          </linearGradient>
        </defs>

        {/* Board surface fill. */}
        <rect
          x="0"
          y="0"
          width={width}
          height={height}
          fill="url(#xq-board-surface)"
        />

        {lines}
        {drawPalace(0)}
        {drawPalace(7)}
        {positionMarks}

        {/* River legend. */}
        <text
          x={width / 2}
          y={height / 2}
          textAnchor="middle"
          dominantBaseline="central"
          fontSize={cellSize * 0.58}
          fill="var(--xq-board-ink-muted)"
          opacity={0.55}
          letterSpacing={cellSize * 0.1}
          style={{
            fontFamily:
              "ui-serif, 'Songti SC', 'Noto Serif CJK SC', 'Source Han Serif SC', serif",
            fontStyle: 'italic',
          }}
        >
          楚 河 漢 界
        </text>

        {overlays}
        {hitboxes}
        {pieces}
        {labels}
      </svg>
    </div>
  )
}
