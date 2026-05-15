export type Player = 'red' | 'black'

export type Action = {
  from: number // 0..89
  to: number // 0..89
}

export type BoardCell = number // -7..-1, 0, 1..7

export interface Snapshot {
  board: Int8Array // length 90, row-major
  currentPlayer: Player
  currentRound: number
  lastPlayer: Player | null
  lastAction: Action | null
  validActions: Action[] // engine ordering, do not re-sort
  isOver: boolean
  score: { red: number; black: number }
  repeatCount: number // 1..3
  inCheck: boolean
  canonicalBoard: Int8Array // useful for debug panel
}

export interface XqEngine {
  readonly snapshot: Snapshot

  applyAction: (action: Action) => void
  undoLastAction: () => void
  reset: (startingPlayer?: Player) => void

  actionToString: (a: Action) => string
  actionFromString: (s: string) => Action | { error: string }
  boardReadableString: () => string

  // Debug methods
  getDebugStats: () => {
    stateVectorLength: number
    policyVectorLength: number
    variantCount: number
    variantActionCounts: number[]
  } | null
}

export type MoveProvider = (snapshot: Snapshot) => Promise<Action>
