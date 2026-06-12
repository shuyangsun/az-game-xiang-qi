# Xiang Qi Project Memory

Date: 2026-06-12  
Status: active index  
Area: documentation, game design, API contract, rules, GUI  
Sources: [README.md](../README.md), [AGENTS.md](../AGENTS.md), and the files under
[`memory/`](./)

## Summary

This `memory/` directory is the primary documentation corpus for the
`az-game-xiang-qi` Xiang Qi implementation. It records the AlphaZero API
contract, Xiang Qi rules, `XqGame` design choices, serializer and augmentation
strategy, GUI architecture, task history, and test coverage. Start here when
retrieving context for `include/xq/`, `src/xq/`, `tests/unit/`, `src/main.cc`, or
the browser GUI under `gui/`.

## Core References

| Topic | Read this | Retrieval anchors |
| --- | --- | --- |
| Repository rules | [constitution.md](./constitution.md) | code style, markdown code blocks, reviewing-code |
| Task history | [tasks.md](./tasks.md) | task index, completed implementation work, related files |
| AlphaZero API | [api_contract.md](./api_contract.md) | `::az::game::api::Game`, `XqGame`, `ValidActionsInto`, `PolicyIndex`, serializers |
| API defaults | [defaults.md](./defaults.md) | `DefaultPolicyOutputSerializer`, `DefaultPolicyOutputDeserializer`, board/action/player aliases |
| History buffer | [history_lookback.md](./history_lookback.md) | `kHistoryLookback`, `RingBufferView`, serializer past states |
| MCTS constraints | [mcts_constraints.md](./mcts_constraints.md) | allocation-free `ApplyActionInPlace`, `UndoLastAction`, deterministic actions |
| API migrations | [migrations.md](./migrations.md) | alpha-zero-api v0.2.0, v0.2.1 migration status |

## Game Rules And Design

| Topic | Read this | Retrieval anchors |
| --- | --- | --- |
| Xiang Qi rules overview | [game_rules.md](./game_rules.md) | players, initial setup, turns, termination, scoring |
| Board rules | [game_rules_details/board.md](./game_rules_details/board.md) | 9x10 board, row/col coordinates, river, palace, starting position |
| Piece movement | [game_rules_details/pieces.md](./game_rules_details/pieces.md) | General, Advisor, Elephant, Horse, Chariot, Cannon, Soldier |
| Special rules | [game_rules_details/special_rules.md](./game_rules_details/special_rules.md) | check, Flying General, stalemate, threefold repetition, move limit |
| Termination reference | [game_rules_details/termination.md](./game_rules_details/termination.md) | `IsOver`, `GetScore`, checkmate, stalemate, draw priority |
| `XqGame` design | [game_design.md](./game_design.md) | `XqB`, `XqA`, `XqP`, `XqError`, `kPolicySize`, `kMaxRounds` |
| Board encoding | [game_design_details/board_encoding.md](./game_design_details/board_encoding.md) | signed piece codes, `CanonicalBoard`, 15-plane dense input |
| Action encoding | [game_design_details/action_encoding.md](./game_design_details/action_encoding.md) | `XqA{from,to}`, sentinel, `PolicyIndex`, `ActionToString` |
| Repetition tracking | [game_design_details/repetition.md](./game_design_details/repetition.md) | Zobrist hash, `position_history_`, snapshot constructor limits |

## Runtime, Training, And GUI

| Topic | Read this | Retrieval anchors |
| --- | --- | --- |
| Augmentation | [augmentation_strategy.md](./augmentation_strategy.md) | `XqAugmentation`, `AugmentAll`, inference `Interpret`, training augment |
| REPL binary | [main_binary.md](./main_binary.md) | `src/main.cc`, actions command, serializer debug lines, augmentation debug lines |
| GUI design | [gui_design.md](./gui_design.md) | WASM bridge, `useXqGame`, `GameKit`, debug panel, upstream reuse |
| Unit test coverage | [unittest_checklists.md](./unittest_checklists.md) | functional requirements, `FR-*`, `tests/unit` coverage |

## Source Map

- Public C++ headers live under [`include/xq/`](../include/xq/); the
  `XqGame` API contract is centered on [`include/xq/game.h`](../include/xq/game.h).
- C++ implementation files live under [`src/xq/`](../src/xq/), with the
  interactive REPL in [`src/main.cc`](../src/main.cc).
- Unit tests live under [`tests/unit/`](../tests/unit/) and are mapped back to
  functional requirements in [unittest_checklists.md](./unittest_checklists.md).
- The browser GUI lives under [`gui/`](../gui/) and is documented by
  [gui/README.md](../gui/README.md) plus [gui_design.md](./gui_design.md).

