# History Lookback

How to choose `XqGame::kHistoryLookback` and how the
engine-owned `RingBuffer<G>` interacts with the serializer.

## Picking a value

`kHistoryLookback` is the number of **past** states the serializer
needs visibility into. The current state is always available as the
`game` parameter; the lookback only counts what came before it.

| Game class | Typical `kHistoryLookback` | Why |
| --- | --- | --- |
| Markov board games (chess if you ignore three-fold repetition; Go if you ignore super-ko; tic-tac-toe; Connect Four) | `0` | Network input is fully determined by the current position. |
| Chess (paper-faithful) | `7` | AlphaZero chess uses 8 time steps total — current + 7 past. |
| Go (paper-faithful) | `7` | AlphaGo Zero uses 8 own-stone planes + 8 opponent planes. |
| Atari-style frame stacking | `3`–`7` | Velocity / direction is recoverable only with multiple frames. |
| Card games with hidden information | depends | Often 0 — the public state plus the current player's hand is Markov. |

When in doubt, start with `0` and add lookback only if your network
input genuinely needs it. Larger lookback inflates the per-node memory
footprint of the engine's history buffer (`std::array<G, N>`), which
matters for games with large `sizeof(G)`.

## What the engine guarantees

The engine maintains a `RingBuffer<G, std::array<G, kHistoryLookback>>`
and passes a `RingBufferView<G>` into every
`SerializeCurrentState(game, history)` call. Guarantees:

- `history.Size() <= kHistoryLookback`. During the first few plies of a
  game the buffer is partially full; the serializer must handle that
  shorter prefix gracefully (e.g., by zero-padding the lookback
  channels).
- Index 0 is the **most recent** past state preceding `game`. Index
  `Size() - 1` is the oldest still in the window.
- `game` itself is **not** in the view. Callers that want "current +
  past" should encode `game` separately and then iterate the view.
- For `kHistoryLookback == 0` the view is always empty and the storage
  occupies essentially no space (`std::array<G, 0>`).

## Implications for the game class

`kHistoryLookback` does **not** require the game to track its own past.
The engine owns the buffer; the game stays Markov-shaped. Only put
history inside `XqGame` itself if game *rules*
need it — e.g. three-fold repetition counters in chess, the ko-point
for Go, the discard pile in a card game. Those go in `board_t` (or in
private fields), not in a parallel ring buffer.

## REPL wiring (for reference)

`src/main.cc` constructs the buffer using the type alias

```cpp
using GameHistory =
    RingBuffer<XqGame,
               std::array<XqGame,
                          XqGame::kHistoryLookback>>;
```

and pushes the current state onto the buffer **before** calling
`game.ApplyActionInPlace(action)`. So the next round's serializer sees
this round's state at index 0. If you change `kHistoryLookback`, the
buffer resizes automatically — no REPL changes needed.
