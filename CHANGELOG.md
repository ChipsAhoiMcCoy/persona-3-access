# Changelog

## 0.1.0-alpha.2

### Stability
- The mod now checks which version of the game it is running on. On anything but the current Steam version, such as the demo, it says so and turns its menus off, because reading them there caused crashes and frozen games. Messages and the field still work.
- Menu lines no longer repeat by themselves. A line is not said again until you press something or something else speaks, and holding a key at the end of a list stays quiet.
- Holding a confirm button now counts as one press.
- Quitting the game no longer makes the mod say false things, like "April 1" and every Social Link reversed, or fault on the way out.

### Battles
- New: anyone in focus says their ailments with their health, such as "Jin, 96%, Downed, Poisoned".
- New: a details list for whoever is in focus. Ctrl+Down or the right stick down moves through it line by line, and Ctrl+Up or the stick up goes back. It holds name and level, HP, SP, ailments and buffs with their turns, guarding, the affinities you have found, and for a party member their Persona, stats and Theurgy gauge.
- Your first target move on a battle's first turn is no longer lost.
- With one enemy left, pressing left or right says who it is.
- A skill, item or Guard you chose yourself is no longer read back. What enemies, and members acting on their own, do is still read.
- A false action after an enemy's Guard is gone, and a counter now names the character who countered.
- Several weaknesses are separated by commas, and affinity lines no longer end on a missing attack name.
- Party members' levels in battle are now correct.

### Victory screen
- "Victory" now waits for the last blow's news instead of cutting it off.
- The EXP, the money and each item are now their own lines, read in turn, and you can go back through them with the arrows.
- Level-ups that slide in no longer move you off the line you are on.
- A Persona's status page reads its level, the skill learned and the stats that rose.
- The hero's level-up page is read again after a notice cuts it off, and a press stops the reading without carrying into the next battle.
- A member's max HP and SP are no longer read at level-up, because the numbers were out of date.

### Menus
- Menus now wait for the game's opening animations. A menu speaks once it can take input, and presses made before then are dropped, as the game drops them.
- Titles such as "Skills" and "Choose target" are no longer cut off.
- Fusion Search and Special Fusion make one tick per move instead of two.
- Config reads a new list once, from the right row.
- Equip: the hints and the details view's scroll arrows now match the game.

### Exploring
- The party's voiced chatter is no longer read over the voices, so it stops cutting off lines like "Thebel, floor 4". This includes the Reaper's warning, which you still hear from the game. A line with no voice is still read.
- Camp menu tabs, such as Consumable Items, are no longer announced as objectives.

## 0.1.0-alpha.1

The first public release. This is an alpha: expect bugs, and please report them.

- Speech for dialogue, choices, notices, menus, battles, shops, Social Links, the Velvet Room, calendar, mail, saves and more, through NVDA, JAWS or Windows speech.
- Every menu is navigable with the arrow keys or D-pad, with positions, descriptions and type-ahead where it applies. The game's highlight follows.
- Getting around: points of interest, auto-walk, a guidance beacon, nearby chimes, wall tones and Shadow alerts, with optional 3D sound for headphones.
- Written descriptions of every area, following time of day and season.
- Audio description for all 42 animated story movies, by a separate narrator voice.
- Spoken descriptions of story cut-ins and full-screen pictures.
- Full controller support for the mod's keys: hold both triggers.
- A settings menu on F2, or both triggers and View.

See [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for what is missing or untested.
