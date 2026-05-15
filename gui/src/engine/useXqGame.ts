import { useState, useCallback, useMemo, useEffect } from 'react'
import type { Action, Player, Snapshot, XqEngine, MoveProvider } from './types'
import { loadXqWasm, type XqWasmConfig } from './xq-wasm'
import { XqGameWrapper } from './xq-game'

export interface UseXqGameOptions {
  wasmConfig?: XqWasmConfig
  startingPlayer?: Player
  moveProvider?: {
    red?: MoveProvider
    black?: MoveProvider
  }
}

export function useXqGame(options: UseXqGameOptions = {}) {
  const [engine, setEngine] = useState<XqGameWrapper | null>(null)
  const [snapshot, setSnapshot] = useState<Snapshot | null>(null)
  const [error, setError] = useState<Error | null>(null)

  // Initialize WASM module and engine
  useEffect(() => {
    let mounted = true

    loadXqWasm(options.wasmConfig)
      .then((module) => {
        if (mounted) {
          const wrapper = new XqGameWrapper(module, options.startingPlayer)
          setEngine(wrapper)
          setSnapshot(wrapper.snapshot)
        }
      })
      .catch((err) => {
        if (mounted) setError(err)
      })

    return () => {
      mounted = false
      // We don't necessarily want to destroy the engine on unmount if it's meant to be long-lived,
      // but for a React component lifecycle it's safe to clean up memory.
      setEngine((prev) => {
        if (prev) prev.destroy()
        return null
      })
    }
  }, [options.wasmConfig?.wasmUrl, options.wasmConfig?.jsUrl])

  // Expose memoized callbacks
  const applyAction = useCallback(
    (action: Action) => {
      if (!engine) return
      try {
        engine.applyAction(action)
        setSnapshot(engine.snapshot)
      } catch (e) {
        console.error(e)
      }
    },
    [engine],
  )

  const undoLastAction = useCallback(() => {
    if (!engine) return
    engine.undoLastAction()
    setSnapshot(engine.snapshot)
  }, [engine])

  const reset = useCallback(
    (startingPlayer: Player = 'red') => {
      if (!engine) return
      engine.reset(startingPlayer)
      setSnapshot(engine.snapshot)
    },
    [engine],
  )

  // Handle move providers for AI
  useEffect(() => {
    if (!snapshot || !engine || snapshot.isOver) return

    const provider = options.moveProvider?.[snapshot.currentPlayer]
    if (provider) {
      let isStale = false
      provider(snapshot)
        .then((action) => {
          if (!isStale) {
            applyAction(action)
          }
        })
        .catch((err) => {
          console.error('Move provider failed:', err)
        })

      return () => {
        isStale = true
      }
    }
  }, [snapshot, engine, applyAction, options.moveProvider])

  return {
    engine,
    snapshot,
    error,
    applyAction,
    undoLastAction,
    reset,
    isReady: !!snapshot,
  }
}
