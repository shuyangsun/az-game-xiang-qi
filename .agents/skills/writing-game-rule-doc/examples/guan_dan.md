# Guan Dan rules

This guide describes the standard four-player partnership version of Guan Dan
(掼蛋).

## Basic setup

- Four players sit in clockwise order.
- Partners sit opposite each other and work as a team.
- Guan Dan uses two full 54-card decks, for a total of 108 cards, 27 each.
- Suits matter, especially for straight flushes and for deciding which cards
  are wild.

## Levels and wild cards

- Each team keeps its own level from hand to hand, and the two teams may be on
  different levels.
- At the start of the game, both teams begin with `2` being the current level
  card. After that, the next hand uses the winning team's new level card as the
  current level card.
- Teams level up cards through `3`, `4`, and so on up to ace (`A`).
- You cannot skip `A`. A team must play an `A`-level hand before it can win the
  match.
- The match-winning result is passing `A` (`过A`). Older official Jiangsu
  material also refers to that terminal result as `A+`, but it is not a normal
  extra level that you keep playing through.
- The current level card ranks above the ordinary non-joker cards when hands
  are compared.
- Big joker is above the small joker.
- The heart copy of the current level card is wild. Because two decks are used,
  there can be at most two wild cards in play.
- Wild cards can be declared as any non-joker card.

## How a trick works

- Play moves clockwise.
- A player who has already used all of their cards is skipped.
- To start a trick, the leader must play a legal hand. You cannot pass on a
  fresh trick.
- After that, each active player either plays a higher legal hand or passes.
- The hand to beat is always the last non-pass play.
- When every other active player has passed, the last player who played wins
  the trick and leads the next one.
- Exception: if a player goes out on a play and both opponents pass, that
  player's partner leads the next trick.
- Passes only matter for the current trick. They are cleared when the next
  trick starts.

## Announcing remaining cards

- In live play, a player who ends a play with 10 or fewer cards announces the
  exact number remaining.
- Competitive rules usually treat this as a one-time announcement and do not
  allow repeated reminders or repeated questions about that count.
- For this project's game implementation, you may ignore this rule because the
  complete game history is visible.

## Legal hands

- single: one card (单张)
- pair: two cards of the same rank (对子)
- triple: three cards of the same rank (三张)
- full house: a triple plus a pair (三带二)
- straight: five consecutive ranks (顺子)
- tube: three consecutive pairs (三连对)
- plate: two consecutive triples (钢板)
- straight flush: a five-card straight in one suit (同花顺)
- bomb: four or more cards of the same rank (炸弹)
- joker bomb: all four jokers (王炸)

An ordinary bomb can be as large as 10 cards: eight natural cards of one rank
plus both wild hearts. This is very rare, but a legal move.

### Other legal actions

- tribute: a pre-hand card exchange based on the previous hand's finish
  order (进贡)
- return tribute: the player who receives a tribute gives back one card to
  that same player (还贡)
- anti-tribute: cancel the tribute step when the big-joker condition is met
  (抗贡)
- pass: decline to play a hand (过牌)

## How Hands Are Compared

- In normal play, you beat a hand with a higher hand of the same type.
- Singles, pairs, and triples are compared by rank.
- Full houses are compared by the rank of the triple.
- In all run-based hands (straight, straight flush, tube, and plate), `A` may
  be used low or high, for example `A-2-3-4-5` and `10-J-Q-K-A`, but runs do
  not wrap, so `Q-K-A-2-3` is not legal.
- Straights and straight flushes are compared by the highest number in the run.
  If the level card appears in the run, use its printed number here, not its
  boosted rank.
- Tubes are compared by the highest pair in the run.
- Plates are compared by the highest triple in the run.
- Bombs are compared first by size, then by rank if the sizes match.
- A joker bomb is the highest possible hand.
- Straight flushes, bombs, and joker bombs are special power hands that can
  beat ordinary hands.
- A straight flush beats a five-bomb, but a six (or higher) bomb beats any
  straight flush.
- Joker bomb beats everything else.

## Wild Cards and Declared Hands

- Wild cards can stand in for other cards when forming a legal hand.
- Because of that, the same set of cards may sometimes be played in more than
  one valid way.
- The player must make clear which hand they are playing. Once that is said,
  the choice is final for that trick.
- Other players only need to beat the declared hand, not every other hand those
  same cards might have formed.

## Going out and scoring the hand

- A player who plays their last card leaves the hand immediately.
- Players who are out no longer take turns, but the order in which players go
  out still matters.
- The finish-order terms are first out / `上游`, second out / `二游`, third out
  / `三游`, and last out / `下游`.
- If partners finish first and second, it is a triple-up (`+3` levels).
- If partners finish first and third, it is a double-up (`+2` levels).
- If partners finish first and fourth, it is a single-up (`+1` level).
- Level gains are capped at `A`. If a result would move a team past `A`, that
  team plays the next hand at `A` instead.

## Passing ace and winning the match

- Guan Dan is won by the first team to pass `A` (`过A`).
- A team playing at level `A` passes `A` only if one teammate finishes first
  and the partner does not finish last.
- In four-player Guan Dan, that means only a `1-2` finish or a `1-3` finish at
  level `A` wins the match.
- A `1-4` finish at level `A` does not pass `A`; the hand ends, but that team
  must play `A` again.
- This document follows the current national competitive rule text and the
  official Jiangsu simplified rule text that define winning as passing `A`.
- Some local or tournament variants add a specific failure rule at `A`: if the
  same team records three consecutive `1-4` finishes while trying to pass `A`,
  that attempt fails and the team resets to level `2`.

## Tribute

- Tribute starts with the second hand, after the deal and before the first
  trick.
- In single tribute, the lower finisher on the losing side gives one card to
  the previous hand's first-place player.
- In double tribute, one team finished third and fourth (`双下游`), so both
  players on that team each give one card.
- A tribute card is the giver's highest eligible single under the current
  single-card rank order, excluding the heart level card, even if this breaks
  another hand.
- If several cards tie for highest, any of them may be given.
- Check anti-tribute before any cards move.
- In single tribute, anti-tribute happens if the tribute giver holds both big
  jokers.
- In double tribute, anti-tribute happens if the two tribute givers together
  hold both big jokers, split across the two hands or both in one hand.
- If anti-tribute happens, no cards are exchanged and the previous hand's
  first-place player leads.
- Otherwise, in double tribute the higher tribute goes to first place and the
  lower goes to second place. If they tie, the giver encountered first in
  clockwise order from first place gives to first place.
- Each receiver returns one card to that same giver.
- The returned card must have printed rank `10` or lower. If none exists,
  return the lowest card in the hand.
- After single tribute, the tribute giver leads. After double tribute, the
  giver whose tribute went to first place leads.
