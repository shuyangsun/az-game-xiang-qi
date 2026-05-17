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

  // Debug probes.
  getDebugStats: () => {
    stateVectorLength: number
    policyVectorLength: number
    variantCount: number
    variantActionCounts: number[]
  } | null
  /**
   * Snapshot-keyed debug probe. The `snapshot` argument is unused
   * inside the implementation — it's threaded through so React
   * Compiler infers the snapshot as a real dep of the call and
   * re-runs it on each move/undo. Without the arg the compiler
   * narrows the inferred deps to `engine` alone (snapshot only
   * appears in null-checks) and the probe stays pinned.
   */
  getDebugProbe: (snapshot: Snapshot) => DebugProbe | null
}

export interface DebugProbeVariant {
  index: number
  name: string
  currentPlayer: Player
  validActionCount: number
  /** Length-90 row-major copy; safe to retain. */
  board: Int8Array
  stateVectorLength: number
  stateRoundtripOk: boolean
}

export interface DebugProbe {
  validActionCount: number
  stateVectorLength: number
  stateRoundtripOk: boolean
  /** Length-90 canonical board copy. */
  canonicalBoard: Int8Array
  variants: DebugProbeVariant[]
}

export type MoveProvider = (snapshot: Snapshot) => Promise<Action>
