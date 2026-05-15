import React, { useState } from 'react'
import type { XqEngine } from '../../engine'

export interface DebugPanelProps {
  engine: XqEngine
  hoveredAction: number | null // index in validActions
}

export function DebugPanel({ engine, hoveredAction }: DebugPanelProps) {
  const [showCanonical, setShowCanonical] = useState(false)
  const [expanded, setExpanded] = useState(false)

  const snap = engine.snapshot
  const stats = engine.getDebugStats()

  if (!expanded) {
    return (
      <div
        className="bg-slate-800 text-slate-300 p-2 text-sm flex justify-between items-center cursor-pointer hover:bg-slate-700"
        onClick={() => setExpanded(true)}
      >
        <span className="font-semibold text-slate-100">
          [Debug Panel] Click to expand
        </span>
      </div>
    )
  }

  const renderBoardGrid = (board: Int8Array) => {
    const rows = []
    for (let r = 0; r < 10; r++) {
      const cols = []
      for (let c = 0; c < 9; c++) {
        const val = board[r * 9 + c]
        cols.push(
          <div
            key={c}
            className={`w-8 h-8 flex items-center justify-center border border-slate-700 ${val > 0 ? 'text-red-400' : val < 0 ? 'text-blue-400' : 'text-slate-600'}`}
          >
            {val !== 0 ? val : '.'}
          </div>,
        )
      }
      rows.push(
        <div key={r} className="flex">
          {cols}
        </div>,
      )
    }
    return (
      <div className="flex flex-col border border-slate-700 font-mono text-xs">
        {rows}
      </div>
    )
  }

  return (
    <div className="bg-slate-900 text-slate-300 p-4 text-sm flex flex-col gap-6">
      <div className="flex justify-between items-center border-b border-slate-700 pb-2">
        <h3 className="font-bold text-lg text-slate-100">Engine Debug Panel</h3>
        <button
          className="px-3 py-1 bg-slate-700 hover:bg-slate-600 rounded text-slate-200"
          onClick={() => setExpanded(false)}
        >
          Close
        </button>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        <div>
          <div className="flex items-center gap-4 mb-2">
            <h4 className="font-semibold text-slate-200">Raw Board Codes</h4>
            <label className="flex items-center gap-2 text-xs">
              <input
                type="checkbox"
                checked={showCanonical}
                onChange={(e) => setShowCanonical(e.target.checked)}
              />
              Canonical View
            </label>
          </div>
          {renderBoardGrid(showCanonical ? snap.canonicalBoard : snap.board)}
        </div>

        <div className="flex flex-col gap-4">
          <div>
            <h4 className="font-semibold text-slate-200 mb-2">
              Valid Actions ({snap.validActions.length})
            </h4>
            <div className="max-h-40 overflow-y-auto bg-slate-800 border border-slate-700 p-2 font-mono text-xs">
              {snap.validActions.map((a, i) => (
                <span
                  key={i}
                  className={`inline-block w-16 ${hoveredAction === i ? 'bg-slate-600 text-white' : ''}`}
                >
                  {engine.actionToString(a)}
                </span>
              ))}
            </div>
          </div>

          <div>
            <h4 className="font-semibold text-slate-200 mb-2">
              Serialization & Augmentation Probe
            </h4>
            {stats ? (
              <div className="bg-slate-800 border border-slate-700 p-2 font-mono text-xs space-y-1">
                <div>
                  State Vector Length:{' '}
                  <span className="text-green-400">
                    {stats.stateVectorLength}
                  </span>
                </div>
                <div>
                  Policy Vector Length:{' '}
                  <span className="text-green-400">
                    {stats.policyVectorLength}
                  </span>
                </div>
                <div>
                  Variant Count:{' '}
                  <span className="text-green-400">{stats.variantCount}</span>
                </div>
                <div>
                  Variant Action Counts:{' '}
                  <span className="text-green-400">
                    {stats.variantActionCounts.join(', ')}
                  </span>
                </div>
              </div>
            ) : (
              <div className="text-amber-400 italic text-xs">
                Not available (built without serializer bindings)
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  )
}
