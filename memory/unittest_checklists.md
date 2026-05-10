# Xiang Qi Unittest Checklists

TODO(TASK-REVIEW-GAME-FR-COV): review functional requirements test
coverage.

Each `FR-*` is a functional requirement derived from
[game_rules.md](./game_rules.md) and its detail files, plus the
API-contract floor below. Tick the box when an automated unit test
covers it.

## Game-rule FRs

### Players and sides ([game_rules.md](./game_rules.md))

- [x] **FR-PLAYER-RED-FIRST** — `XqGame()` (default and explicit)
  starts with `CurrentPlayer() == kRed` (`false`).
- [x] **FR-PLAYER-EXPLICIT** — `XqGame(kBlack)` starts with
  `CurrentPlayer() == kBlack` (`true`).

### Initial setup ([board.md](./game_rules_details/board.md))

- [x] **FR-INIT-RED-BACK** — Row 0 of the default board contains
  Red back-rank pieces in order: Chariot, Horse, Elephant, Advisor,
  General, Advisor, Elephant, Horse, Chariot.
- [x] **FR-INIT-BLACK-BACK** — Row 9 of the default board contains
  Black back-rank pieces in the same order, all negated.
- [x] **FR-INIT-CANNONS** — Cannons at row 2 cols {1, 7} (Red), row
  7 cols {1, 7} (Black).
- [x] **FR-INIT-SOLDIERS** — Soldiers at row 3 cols {0,2,4,6,8}
  (Red), row 6 cols {0,2,4,6,8} (Black).
- [x] **FR-INIT-EMPTY-CELLS** — Every other cell of the default
  board is `0`.
- [x] **FR-INIT-ROUND-ZERO** — `CurrentRound() == 0` on a
  freshly-constructed game.
- [x] **FR-INIT-NO-LAST** — `LastAction()` and `LastPlayer()` are
  `std::nullopt` on a freshly-constructed game.

### Piece movement ([pieces.md](./game_rules_details/pieces.md))

#### General

- [x] **FR-GEN-ORTH-1** — General can move 1 step orthogonally
  inside palace.
- [x] **FR-GEN-PALACE** — General cannot leave its palace
  (rows 0-2 cols 3-5 for Red; rows 7-9 cols 3-5 for Black).
- [x] **FR-GEN-FLYING** — A move that leaves the two Generals on
  the same file with no pieces between is illegal.
- [ ] **FR-GEN-CAPTURE** — General captures by displacement.

#### Advisor

- [x] **FR-ADV-DIAG-1** — Advisor moves exactly 1 step diagonally.
- [x] **FR-ADV-PALACE** — Advisor cannot leave its palace.
- [x] **FR-ADV-REACHABLE** — Advisor reaches only the 5 valid
  palace points (4 corners + center).

#### Elephant

- [x] **FR-ELE-DIAG-2** — Elephant moves exactly 2 steps
  diagonally.
- [x] **FR-ELE-NO-RIVER** — Red Elephant stays on rows 0-4; Black
  Elephant stays on rows 5-9.
- [x] **FR-ELE-EYE-BLOCKED** — Elephant move is blocked if the
  diagonal-1 midpoint is occupied.
- [ ] **FR-ELE-CAPTURE** — Elephant captures by displacement when
  the eye is empty.

#### Horse

- [x] **FR-HORSE-L** — Horse moves to an L-shape destination
  (knight's offsets).
- [x] **FR-HORSE-LEG-BLOCKED** — Horse move is blocked if the
  orthogonal-leg square is occupied.
- [ ] **FR-HORSE-CAPTURE** — Horse captures by displacement.

#### Chariot

- [x] **FR-CHAR-LINE** — Chariot moves any number of empty
  intersections orthogonally.
- [x] **FR-CHAR-NO-JUMP** — Chariot cannot pass over any piece.
- [x] **FR-CHAR-CAPTURE** — Chariot captures the first enemy in
  line of sight.

#### Cannon

- [x] **FR-CANNON-MOVE** — Without capture, Cannon moves like a
  Chariot through empty cells.
- [x] **FR-CANNON-NEED-SCREEN** — Cannon cannot capture without a
  screen.
- [x] **FR-CANNON-ONE-SCREEN** — Cannon capture requires exactly
  one piece (friend or foe) between source and target.
- [x] **FR-CANNON-TARGET-ENEMY** — Cannon capture target must be
  enemy.

#### Soldier

- [x] **FR-SOLDIER-FORWARD-PRE** — Before crossing the river,
  Soldier moves only 1 step forward (Red: row+1, Black: row-1).
- [x] **FR-SOLDIER-SIDEWAYS-POST** — After crossing the river,
  Soldier may also move 1 step left or right.
- [x] **FR-SOLDIER-NO-BACK** — Soldier never moves backward.
- [ ] **FR-SOLDIER-NO-DIAG** — Soldier never moves diagonally.
- [ ] **FR-SOLDIER-CAPTURE** — Soldier captures by displacement.
- [x] **FR-SOLDIER-NO-PROMOTION** — Soldier remains a Soldier on
  reaching the back rank.

### Captures and turns

- [x] **FR-CAPTURE-REMOVE** — Moving onto an enemy piece removes
  the captured piece from the board.
- [x] **FR-CAPTURE-NO-FRIEND** — A move onto a friendly piece is
  not a valid action.
- [x] **FR-TURN-ALTERNATE** — `CurrentPlayer()` flips after every
  `ApplyActionInPlace`.
- [x] **FR-TURN-INCREMENT** — `CurrentRound()` increases by 1
  after every `ApplyActionInPlace`.
- [x] **FR-NO-PASS** — `ValidActions()` never contains the
  no-action sentinel `XqA{kBoardCells, kBoardCells}`.

### Special rules ([special_rules.md](./game_rules_details/special_rules.md))

- [ ] **FR-CHECK-DETECTED** — A position where the side-to-move's
  General is attacked is reported as `in check` (via test
  helper or direct exposure).
- [x] **FR-MOVE-NO-SELF-CHECK** — A move that would leave the
  moving side's General in check is excluded from `ValidActions()`.
- [x] **FR-MOVE-NO-FLYING** — A move that would result in a
  face-to-face Generals position is excluded from
  `ValidActions()`.
- [ ] **FR-CHECKMATE-LOSS** — In a checkmate position,
  `IsOver() == true`, side-to-move score = `-1`, opponent = `+1`.
- [ ] **FR-STALEMATE-LOSS** — In a stalemate position (no legal
  moves, not in check), `IsOver() == true`, side-to-move score =
  `-1`, opponent = `+1` (Asian rule).
- [ ] **FR-REPETITION-DRAW** — A position recurring three times
  yields `IsOver() == true` with both scores = `0`.
- [ ] **FR-REPETITION-UNDO** — Undoing the move that triggered
  threefold repetition restores `IsOver() == false`.
- [x] **FR-MAX-ROUNDS-DRAW** — `IsOver()` returns `true` once
  `CurrentRound() >= *kMaxRounds`, with both scores = `0`.

### Termination ([termination.md](./game_rules_details/termination.md))

- [ ] **FR-TERM-PRIORITY** — Termination ordering matches
  documentation: checkmate > stalemate > threefold > max-rounds
  (i.e., a position that is both threefold and checkmate is
  scored as checkmate).
- [x] **FR-TERM-EMPTY-VALID** — `ValidActions()` is empty if and
  only if `IsOver()` returns `true`.
- [x] **FR-SCORE-RANGE** — `GetScore(player)` is in `[-1.0, +1.0]`
  for every reachable state and every player.

## API-contract FRs ([api_contract.md](./api_contract.md))

- [x] **FR-API-CONCEPT** — `static_assert(::az::game::api::Game<XqGame>)`
  compiles (verified by build, not a runtime test).
- [x] **FR-API-BOARD-STR-NONEMPTY** — `BoardReadableString()`
  returns a non-empty string and is distinct across the initial,
  a mid-game, and a terminal position.
- [x] **FR-API-ACTION-TO-STR-DETERMINISTIC** — `ActionToString` is
  deterministic and produces a distinct string per distinct action.
- [x] **FR-API-ACTION-ROUNDTRIP** — `ActionFromString(ActionToString(a))`
  returns `a` for every action `a` reachable through legal play.
- [x] **FR-API-ACTION-FROM-STR-ERR** — `ActionFromString` returns
  the appropriate `XqError` (not the unknown default) for empty,
  malformed, out-of-range column, and out-of-range row inputs.
- [x] **FR-API-POLICY-BIJECTION** — `PolicyIndex` is a bijection
  from `XqA{from, to}` (`from`, `to` in `[0, 90)`) into
  `[0, kPolicySize)`.
- [x] **FR-API-VALID-DETERMINISTIC** — `ValidActions()` returns
  the same vector when called twice on the same state (no time /
  RNG / call-count dependency).
- [x] **FR-API-VALID-NO-DUPES** — `ValidActions()` contains no
  duplicates.
- [x] **FR-API-LAST-EMPTY** — `LastAction()` and `LastPlayer()`
  are `std::nullopt` before any apply.
- [x] **FR-API-LAST-AFTER-APPLY** — Both are populated after the
  first `ApplyActionInPlace`.

## Mutation contract ([mcts_constraints.md](./mcts_constraints.md))

- [x] **FR-MCTS-APPLY-UNDO** — `Apply(a) -> Undo()` returns the
  game to its previous observable state (board, current_player,
  round, LastAction, LastPlayer, IsOver, GetScore, ValidActions).
- [x] **FR-MCTS-DEEP-ROLLOUT** — `Apply(a_1) ... Apply(a_N) ->
  Undo() x N` returns to the original state for any rollout up to
  `*kMaxRounds`.
- [x] **FR-MCTS-COPY-PRESERVES** — Copy-construction preserves
  every observable field including the position-history log.
- [x] **FR-MCTS-MOVE-PRESERVES** — Move-construction preserves
  every observable field of the moved-from instance pre-move.

## Serializer / deserializer contract

- [x] **FR-SER-FIXED-LEN** — `SerializeCurrentState` returns a
  fixed-length vector across the initial, mid-game, and terminal
  states.
- [x] **FR-SER-INPUT-LEN** — `SerializeCurrentState` length
  matches the documented value (15 planes × 90 cells = 1350).
- [x] **FR-SER-EMPTY-HISTORY** — `SerializeCurrentState` works
  with an empty `RingBufferView` (Markov game).
- [x] **FR-SER-ENCODES-PLAYER** — Side-to-move plane reflects
  `CurrentPlayer()`.
- [x] **FR-SER-POLICY-LEN** — `SerializePolicyOutput` length
  equals `kPolicySize + 1 == 8101`.
- [x] **FR-SER-POLICY-Z-FIRST** — `SerializePolicyOutput`
  result[0] equals `target.z`.
- [x] **FR-SER-POLICY-MASK** — Slots not in
  `1 + PolicyIndex(ValidActions()[i])` are zero.
- [x] **FR-DES-WRONG-SIZE** — `Deserialize` returns
  `XqError::kInvalidPolicyOutputSize` when length is wrong.
- [x] **FR-DES-PARALLEL** — `Evaluation.probabilities.size()`
  equals `ValidActions().size()` and corresponds 1:1.
- [x] **FR-DES-VALUE-PASSTHROUGH** — `Evaluation.value` equals
  `output[0]`.
- [x] **FR-DES-ROUNDTRIP** — `Deserialize(SerializePolicyOutput(target))`
  recovers `target.z` and `target.pi` within numerical tolerance.

## Augmenter contract ([augmentation_strategy.md](./augmentation_strategy.md))

- [x] **FR-AUG-CARDINALITY** — `AugmentAll(game).size()` equals
  the number of `XqAugmentation` enum members (= 2).
- [x] **FR-AUG-ORDER** — `AugmentAll(game)[i]` corresponds to
  `static_cast<XqAugmentation>(i)`.
- [x] **FR-AUG-IDENTITY-FIRST** — `AugmentAll(game)[0]` matches
  `game` on every observable field that augmenters preserve
  (board, side to move, valid-action count).
- [x] **FR-AUG-MIRROR-INVOLUTION** — Applying mirror twice yields
  the identity board.
- [x] **FR-AUG-PRESERVES-ACTION-COUNT** — Each augmented variant
  has the same `ValidActions().size()` as the input.
- [x] **FR-AUG-VARIANT-IS-GAME** — Each variant satisfies the
  `Game` concept and produces well-formed `BoardReadableString`,
  `ValidActions`, etc.
- [x] **FR-AUG-INVERSE-ACTION** —
  `InverseTransformAction(transform_action(a, sym), sym) == a`
  for every `sym` and every action `a`.
- [x] **FR-INF-INTERPRET-LEN** — `Interpret(original, ...)` length
  equals `original.ValidActions().size()`.
- [x] **FR-INF-INTERPRET-VALUE** — Identical per-variant values
  averaged by `Interpret` reproduce that value.
- [x] **FR-INF-INTERPRET-PROB** — Identical-uniform per-variant
  probabilities aggregate to identical-uniform output.
- [x] **FR-INF-INTERPRET-ALIGN** — `Interpret`'s probability
  vector aligns with `original.ValidActions()` ordering.
- [x] **FR-TRAIN-AUG-ONE-PER** — Training-time `Augment` returns
  exactly one pair per `XqAugmentation` member.
- [x] **FR-TRAIN-AUG-PI-PERMUTED** — Each augmented `pi` aligns
  with the augmented game's `ValidActions()` (equivariance, not
  invariance).
- [x] **FR-TRAIN-AUG-Z-PRESERVED** — `target.z` is identical
  across all augmented pairs.

## REPL contract ([main_binary.md](./main_binary.md))

The REPL is exercised end-to-end by `PLAY-MAIN-VERIFY`. The unit
tests above pin the behaviors that drive the REPL output.
