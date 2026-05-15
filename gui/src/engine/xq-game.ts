import type { Action, Player, Snapshot, XqEngine } from './types'

export class XqGameWrapper implements XqEngine {
  private module: any
  private gameJs: any
  private serializerJs: any | null
  private currentSnapshot!: Snapshot

  constructor(module: any, startingPlayer: Player = 'red') {
    this.module = module
    const isBlack = startingPlayer === 'black'
    this.gameJs = new module.XqGameJs(isBlack)

    if (module.XqSerializerJs) {
      this.serializerJs = new module.XqSerializerJs()
    } else {
      this.serializerJs = null
    }

    this.updateSnapshot()
  }

  get snapshot(): Snapshot {
    return this.currentSnapshot
  }

  applyAction(action: Action): void {
    // We must find the index of the action in validActions
    const actions = this.currentSnapshot.validActions
    const index = actions.findIndex(
      (a) => a.from === action.from && a.to === action.to,
    )
    if (index === -1) {
      throw new Error(
        `Action from ${action.from} to ${action.to} is not valid.`,
      )
    }

    this.gameJs.applyValidActionByIndex(index)
    this.updateSnapshot()
  }

  undoLastAction(): void {
    if (this.currentSnapshot.currentRound > 0) {
      this.gameJs.undoLastAction()
      this.updateSnapshot()
    }
  }

  reset(startingPlayer: Player = 'red'): void {
    this.gameJs.delete()
    const isBlack = startingPlayer === 'black'
    this.gameJs = new this.module.XqGameJs(isBlack)
    this.updateSnapshot()
  }

  actionToString(a: Action): string {
    return this.gameJs.actionToString({ from: a.from, to: a.to })
  }

  actionFromString(s: string): Action | { error: string } {
    const res = this.gameJs.actionFromString(s)
    if (res.ok) {
      return { from: res.action.from, to: res.action.to }
    }
    return { error: `ErrorCode \${res.errorCode}` }
  }

  boardReadableString(): string {
    return this.gameJs.boardReadableString()
  }

  getDebugStats(): {
    stateVectorLength: number
    policyVectorLength: number
    variantCount: number
    variantActionCounts: number[]
  } | null {
    if (!this.serializerJs) {
      return null
    }

    const state = this.serializerJs.serializeCurrentState(this.gameJs)
    const policy = this.serializerJs.serializePolicyOutput(this.gameJs)
    const aug = this.serializerJs.getAugmentationStats(this.gameJs)

    // length of Float32Array from Emscripten might need to be checked
    const stateVectorLength = state.length
    const policyVectorLength = policy.length

    // aug.variantActionCounts is a JS array returned from Embind val::array
    const variantActionCounts: number[] = []
    for (const count of aug.variantActionCounts) {
      variantActionCounts.push(count)
    }

    return {
      stateVectorLength,
      policyVectorLength,
      variantCount: aug.variantCount,
      variantActionCounts,
    }
  }

  private updateSnapshot() {
    const boardView = this.gameJs.getBoard()
    const board = new Int8Array(boardView.length)
    board.set(boardView)

    const canonicalBoardView = this.gameJs.canonicalBoard()
    const canonicalBoard = new Int8Array(canonicalBoardView.length)
    canonicalBoard.set(canonicalBoardView)

    const validActionsRaw = this.gameJs.validActions()
    const validActions: Action[] = []
    for (const raw of validActionsRaw) {
      validActions.push({
        from: raw.from,
        to: raw.to,
      })
    }

    const currentPlayerRaw = this.gameJs.currentPlayer()
    const currentPlayer: Player = currentPlayerRaw ? 'black' : 'red'

    const lastPlayerRaw = this.gameJs.lastPlayer()
    let lastPlayer: Player | null = null
    if (lastPlayerRaw !== null && lastPlayerRaw !== undefined) {
      lastPlayer = lastPlayerRaw ? 'black' : 'red'
    }

    const lastActionRaw = this.gameJs.lastAction()
    let lastAction: Action | null = null
    if (lastActionRaw !== null && lastActionRaw !== undefined) {
      lastAction = {
        from: lastActionRaw.from,
        to: lastActionRaw.to,
      }
    }

    const isOver = this.gameJs.isOver()

    // In Xiang Qi, if validActions is empty and not over, something is weird.
    // Actually game is over if validActions is empty (stalemate/checkmate).
    // Let's derive inCheck by a simple rule: if it's over, and validActions is 0,
    // we can't easily know if it's check or stalemate without calling GetScore.
    // If GetScore(currentPlayer) == -1, it's a loss. Wait, stalemate is also a loss!
    // We don't have a direct IsInCheck. Let's just default to false unless we know.
    // The design doc says: "derive it in JS by checking validActions.length == 0 && !isOver -- fragile, skip".
    // "Add a bool IsInCheck() const noexcept observer to XqGame ... Recommended."
    // Let me add `IsInCheck` to C++ shortly!
    const inCheck =
      typeof this.gameJs.isInCheck === 'function'
        ? this.gameJs.isInCheck()
        : false

    this.currentSnapshot = {
      board,
      canonicalBoard,
      currentPlayer,
      currentRound: this.gameJs.currentRound(),
      lastPlayer,
      lastAction,
      validActions,
      isOver,
      score: {
        red: this.gameJs.getScore(false),
        black: this.gameJs.getScore(true),
      },
      repeatCount: this.gameJs.currentPositionRepeatCount(),
      inCheck,
    }
  }

  destroy() {
    if (this.gameJs) this.gameJs.delete()
    if (this.serializerJs) this.serializerJs.delete()
  }
}
