import { describe, expect, it } from 'vitest'
import {
  actionFromString,
  actionToString,
  encodeSnapshotForServer,
  xqGameKit,
} from './kit'
import type { Action, Snapshot } from './engine'

describe('actionToString / actionFromString', () => {
  it('encodes a basic action to standard XQ notation', () => {
    // a0-b0: row 0, col 0 -> row 0, col 1. Idx = row * 9 + col.
    expect(actionToString({ from: 0, to: 1 })).toBe('a0-b0')
  })

  it('round-trips a round of action values', () => {
    // Pick a handful of cells spread across the board.
    const cases: Array<Action> = [
      { from: 0, to: 89 },
      { from: 4, to: 45 }, // file e, rank 0 -> file a, rank 5
      { from: 76, to: 67 }, // file d, rank 8 -> file d, rank 7
    ]
    for (const a of cases) {
      const s = actionToString(a)
      const parsed = actionFromString(s)
      expect(parsed).toEqual(a)
    }
  })

  it('rejects malformed action strings', () => {
    expect(actionFromString('not-it')).toEqual({
      error: 'kInvalidActionFormat',
    })
    expect(actionFromString('z0-b0')).toEqual({
      error: 'kInvalidActionFromCell',
    })
    expect(actionFromString('a0-z0')).toEqual({
      error: 'kInvalidActionToCell',
    })
  })

  it('clamps invalid index values to placeholder notation', () => {
    expect(actionToString({ from: -1, to: 0 })).toBe('??-??')
    expect(actionToString({ from: 0, to: 90 })).toBe('??-??')
  })
})

describe('encodeSnapshotForServer', () => {
  it('produces a JSON-serialisable payload of the right shape', () => {
    const board = new Int8Array(90)
    board[0] = 5 // a chariot at a0
    board[89] = -1 // a black general at i9
    const snap: Snapshot = {
      board,
      canonicalBoard: new Int8Array(90),
      currentPlayer: 'red',
      currentRound: 7,
      lastPlayer: null,
      lastAction: null,
      validActions: [],
      isOver: false,
      score: { red: 0, black: 0 },
      repeatCount: 1,
      inCheck: false,
    }
    const payload = encodeSnapshotForServer(snap)
    expect(payload.current_player).toBe('red')
    expect(payload.current_round).toBe(7)
    expect(payload.board.length).toBe(90)
    expect(payload.board[0]).toBe(5)
    expect(payload.board[89]).toBe(-1)
    // Must survive a JSON round-trip — typed arrays serialise to
    // object-keyed garbage, so confirm the payload is plain.
    const round = JSON.parse(JSON.stringify(payload))
    expect(round).toEqual(payload)
  })
})

describe('xqGameKit', () => {
  it('declares the expected meta', () => {
    expect(xqGameKit.meta.gameId).toBe('xiang-qi')
    expect(xqGameKit.meta.policySize).toBe(8100)
    expect(xqGameKit.meta.historyLookback).toBe(0)
    expect(xqGameKit.meta.players).toEqual(['red', 'black'])
    expect(xqGameKit.meta.serverWireVersion).toBe(1)
  })

  it('exposes the helpers as the same functions', () => {
    expect(xqGameKit.actionToString).toBe(actionToString)
    expect(xqGameKit.actionFromString).toBe(actionFromString)
    expect(xqGameKit.encodeSnapshotForServer).toBe(encodeSnapshotForServer)
  })
})
