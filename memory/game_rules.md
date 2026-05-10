# Xiang Qi Rules

Xiang Qi (Chinese Chess) is a two-player abstract strategy board
game played on a 9x10 grid. Each player starts with 16 pieces and
wins by checkmating the opponent's General.

## Contents

- [Players and Sides](#players-and-sides)
- [Smallest Complete Game](#smallest-complete-game)
- [Initial Setup](#initial-setup)
- [Turns and Actions](#turns-and-actions)
- [Termination](#termination)
- [Scoring](#scoring)
- Detailed rules
  - [Board layout](./game_rules_details/board.md)
  - [Pieces and movement](./game_rules_details/pieces.md)
  - [Special rules](./game_rules_details/special_rules.md)
  - [Termination details](./game_rules_details/termination.md)

## Players and Sides

- Two players, no teams.
- Red moves first; Black moves second.
- Pieces are usually marked with Chinese characters; they are
  often referred to by English names (General, Advisor, Elephant,
  Horse, Chariot, Cannon, Soldier).

## Smallest Complete Game

- One full game from the standard starting position to a
  terminal state (checkmate, stalemate, repetition draw, or move
  limit) is a complete game.
- Each game is independent. There is no carryover from earlier
  games (no scoreboard, hands, or persistent state).
- The only constructor input the engine needs is which side
  starts. By tradition this is always Red, but the constructor
  accepts either to keep self-play flexible.

## Initial Setup

- Pieces sit on grid intersections (lines), not inside squares.
- The "river" runs horizontally between rows 4 and 5; it changes
  how Soldiers move and forbids Elephants from crossing.
- Each side has a 3x3 "palace": Red rows 0-2 cols 3-5, Black
  rows 7-9 cols 3-5. Generals and Advisors stay inside.
- See [board.md](./game_rules_details/board.md) for the diagram
  and starting layout.

## Turns and Actions

- Players alternate moves; Red plays first.
- A "move" picks one of the player's own pieces and moves it to
  a destination intersection allowed by that piece's rules.
- If the destination has an enemy piece, that piece is captured
  and removed from the board.
- A side may never capture its own pieces.
- A move is illegal if it would leave the moving player's
  General in check (including the "Flying General" rule that
  forbids the two Generals from facing each other on an open
  file).
- There is no "pass". A player who has no legal moves loses
  (see [termination.md](./game_rules_details/termination.md)).
- Per-piece movement rules live in
  [pieces.md](./game_rules_details/pieces.md).

## Termination

A game ends as soon as any of the following becomes true:

1. **Checkmate** — the side to move is in check and has no legal
   move. The side to move loses.
2. **Stalemate** — the side to move has no legal moves while not
   in check. By Asian rules the side to move loses (this differs
   from Western chess, where stalemate is a draw).
3. **Threefold repetition** — the same position with the same
   side to move occurs three times. Treated as a draw in this
   implementation; perpetual-check / perpetual-chase scoring is
   intentionally simplified, see
   [special_rules.md](./game_rules_details/special_rules.md).
4. **Move limit** — if play reaches the engine's hard ply cap
   (`kMaxRounds`) the game is declared a draw. This exists to
   guarantee self-play termination.

See [termination.md](./game_rules_details/termination.md) for the
exact criteria each test must check.

## Scoring

- Winner gets `+1`, loser gets `-1`, draw is `0` for both sides.
- Scores are returned per player from each player's perspective:
  `GetScore(winner) == +1`, `GetScore(loser) == -1`,
  `GetScore(p) == 0` for either side on a draw or while the game
  is not yet over.
- Scoring depends only on the terminal state — it does not count
  remaining material, captured pieces, or move count.
