import { useEffect, useRef } from 'react'

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
      className={
        className ??
        'flex flex-col border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 w-64 h-full shadow-sm transition-colors'
      }
    >
      <div className="bg-slate-100 dark:bg-slate-900 p-2 font-semibold text-slate-700 dark:text-slate-300 border-b border-slate-200 dark:border-slate-700">
        Move History
      </div>
      <div
        ref={scrollRef}
        className="flex-1 overflow-y-auto p-2 text-sm font-mono space-y-1"
      >
        {moves.length === 0 && (
          <div className="text-slate-400 dark:text-slate-500 italic">No moves yet</div>
        )}
        {moves.map((move, i) => (
          <div key={i} className="px-2 py-1 hover:bg-slate-50 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 rounded transition-colors">
            <span className="text-slate-400 dark:text-slate-500 mr-2 w-6 inline-block text-right">
              {i + 1}.
            </span>
            {move}
          </div>
        ))}
      </div>
    </div>
  )
}
