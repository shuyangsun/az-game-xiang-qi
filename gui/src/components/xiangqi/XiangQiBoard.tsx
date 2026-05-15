import React, { useEffect, useRef, useState } from 'react'
import type { Action } from '../../engine'

export type PieceStyle = 'char' | 'icon'
export type Orientation = 'red-bottom' | 'black-bottom'

export interface XiangQiBoardProps {
  board: Int8Array
  orientation: Orientation
  selectedCell: number | null
  legalTargets: ReadonlySet<number>
  lastMove: Action | null
  inCheckCell: number | null
  onCellClick: (cell: number) => void
  pieceStyle?: PieceStyle
  className?: string
}

const RED_CHARS = ['', '帥', '仕', '相', '馬', '車', '砲', '兵']
const BLACK_CHARS = ['', '將', '士', '象', '傌', '俥', '炮', '卒']

const DEFAULT_CELL_SIZE = 50
const CELL_SIZE_VAR = '--xq-board-cell-size'

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
  pieceStyle = 'char',
  className,
}: XiangQiBoardProps) {
  const containerRef = useRef<HTMLDivElement>(null)
  const [cellSize, setCellSize] = useState<number>(DEFAULT_CELL_SIZE)

  useEffect(() => {
    setCellSize(readCellSize(containerRef.current))
  }, [])

  const padding = 30
  const width = cellSize * 8 + padding * 2
  const height = cellSize * 9 + padding * 2

  const isRedBottom = orientation === 'red-bottom'

  // Map logical (row, col) to SVG (x, y)
  const getCoords = (row: number, col: number) => {
    // If red is bottom, row 0 (red home) is at the bottom (y = 9).
    const visualRow = isRedBottom ? 9 - row : row
    const visualCol = isRedBottom ? col : 8 - col // usually Black is flipped entirely or just Y?
    // Standard Xiang Qi coordinates: Black plays from top. If Red plays from bottom, file 'a' is left.
    // In Chinese notation, right-to-left for own side, but ActionToString uses a-i (0-8) consistently.
    // Let's just flip both axes or just Y?
    // Usually, orientation flips the board 180 degrees. So visualCol = isRedBottom ? col : 8 - col.
    return {
      x: padding + visualCol * cellSize,
      y: padding + visualRow * cellSize,
    }
  }

  const getCellIdx = (row: number, col: number) => row * 9 + col

  // Generate grid lines
  const lines = []
  // Vertical lines
  for (let c = 0; c < 9; c++) {
    const top = getCoords(0, c)
    const bottom = getCoords(9, c)
    if (c === 0 || c === 8) {
      // Outer files go all the way
      lines.push(
        <line
          key={`v${c}`}
          x1={top.x}
          y1={top.y}
          x2={bottom.x}
          y2={bottom.y}
          stroke="black"
        />,
      )
    } else {
      // Inner files break at the river
      const riverTop = getCoords(isRedBottom ? 4 : 5, c)
      const riverBottom = getCoords(isRedBottom ? 5 : 4, c)
      lines.push(
        <line
          key={`vt${c}`}
          x1={top.x}
          y1={top.y}
          x2={riverTop.x}
          y2={riverTop.y}
          stroke="black"
        />,
      )
      lines.push(
        <line
          key={`vb${c}`}
          x1={riverBottom.x}
          y1={riverBottom.y}
          x2={bottom.x}
          y2={bottom.y}
          stroke="black"
        />,
      )
    }
  }
  // Horizontal lines
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
        stroke="black"
      />,
    )
  }

  // Palaces
  const drawPalace = (rStart: number) => {
    const tl = getCoords(rStart, 3)
    const tr = getCoords(rStart, 5)
    const bl = getCoords(rStart + 2, 3)
    const br = getCoords(rStart + 2, 5)
    return (
      <g key={`palace-${rStart}`}>
        <line x1={tl.x} y1={tl.y} x2={br.x} y2={br.y} stroke="black" />
        <line x1={tr.x} y1={tr.y} x2={bl.x} y2={bl.y} stroke="black" />
      </g>
    )
  }

  const pieces = []
  const targets = []
  const hitboxes = []

  for (let row = 0; row < 10; row++) {
    for (let col = 0; col < 9; col++) {
      const idx = getCellIdx(row, col)
      const val = board[idx]
      const { x, y } = getCoords(row, col)

      // Hitbox
      hitboxes.push(
        <rect
          key={`hitbox-${idx}`}
          x={x - cellSize / 2}
          y={y - cellSize / 2}
          width={cellSize}
          height={cellSize}
          fill="transparent"
          cursor="pointer"
          onClick={() => onCellClick(idx)}
        />,
      )

      // Last move highlight
      if (lastMove && (lastMove.from === idx || lastMove.to === idx)) {
        targets.push(
          <rect
            key={`last-${idx}`}
            x={x - cellSize / 2}
            y={y - cellSize / 2}
            width={cellSize}
            height={cellSize}
            className="fill-yellow-300/30 dark:fill-yellow-500/30"
            pointerEvents="none"
          />,
        )
      }

      // Selection highlight
      if (selectedCell === idx) {
        targets.push(
          <circle
            key={`sel-${idx}`}
            cx={x}
            cy={y}
            r={cellSize * 0.45}
            fill="none"
            strokeWidth="3"
            className="stroke-blue-600 dark:stroke-blue-400"
            pointerEvents="none"
          />,
        )
      }

      // Legal target highlight
      if (legalTargets.has(idx)) {
        if (val === 0) {
          targets.push(
            <circle
              key={`tgt-${idx}`}
              cx={x}
              cy={y}
              r={cellSize * 0.15}
              className="fill-green-600 dark:fill-green-500"
              pointerEvents="none"
            />,
          )
        } else {
          targets.push(
            <circle
              key={`tgt-${idx}`}
              cx={x}
              cy={y}
              r={cellSize * 0.45}
              fill="none"
              strokeWidth="4"
              className="stroke-green-600 dark:stroke-green-500"
              pointerEvents="none"
            />,
          )
        }
      }

      // Piece
      if (val !== 0) {
        const isRed = val > 0
        const absVal = Math.abs(val)
        const char = isRed ? RED_CHARS[absVal] : BLACK_CHARS[absVal]
        const isCheck = inCheckCell === idx

        pieces.push(
          <g key={`piece-${idx}`} style={{ pointerEvents: 'none' }}>
            <circle
              cx={x}
              cy={y}
              r={cellSize * 0.4}
              strokeWidth="2"
              className={`fill-[#ffe4b5] dark:fill-slate-800 ${isRed ? 'stroke-red-600 dark:stroke-red-500' : 'stroke-slate-900 dark:stroke-slate-300'}`}
            />
            {isCheck && (
              <circle
                cx={x}
                cy={y}
                r={cellSize * 0.4}
                fill="none"
                strokeWidth="6"
                className="stroke-red-500/80 animate-pulse"
              />
            )}
            <text
              x={x}
              y={y}
              textAnchor="middle"
              dominantBaseline="central"
              fontSize={cellSize * 0.5}
              fontWeight="bold"
              className={isRed ? 'fill-red-600 dark:fill-red-500' : 'fill-slate-900 dark:fill-slate-300'}
            >
              {char}
            </text>
          </g>,
        )
      }
    }
  }

  // Labels
  const labels = []
  for (let c = 0; c < 9; c++) {
    const x = padding + (isRedBottom ? c : 8 - c) * cellSize
    labels.push(
      <text
        key={`l-col-${c}`}
        x={x}
        y={height - 5}
        textAnchor="middle"
        fontSize="12"
        className="fill-slate-600 dark:fill-slate-400"
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
        x={5}
        y={y}
        dominantBaseline="central"
        fontSize="12"
        className="fill-slate-600 dark:fill-slate-400"
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
        width={width}
        height={height}
        className="bg-orange-100 dark:bg-slate-800 rounded shadow-md border border-orange-300 dark:border-slate-700 transition-colors"
      >
        {lines}
        {drawPalace(0)}
        {drawPalace(7)}

        {/* River text */}
        <text
          x={width / 2}
          y={height / 2}
          textAnchor="middle"
          dominantBaseline="central"
          fontSize="30"
          className="fill-slate-500/60 dark:fill-slate-400/50"
        >
          楚 河 漢 界
        </text>

        {targets}
        {hitboxes}
        {pieces}
        {labels}
      </svg>
    </div>
  )
}
