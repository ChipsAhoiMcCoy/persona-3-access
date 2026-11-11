# Changelog

## Unreleased

### Less to listen to
- A battle turn opens with a shorter line: "Junpei, HP 99, SP 17. Cowardly Maya, 35%, Down, weak to Slash, Fire". The HP and SP maxima, the word "Target" and the target's place are gone from it; moving the target still says its place.
- Affinities drop the word "Attack": "weak to Fire, Ice" instead of "weak to Fire Attack, Ice Attack". "Downed" is now "Down".
- The battle skill list reads name and cost only ("Agi, 3 SP, 2 of 3"). What the skill does is one flick away: Ctrl+Down or the right stick down reads it, then the acting member's details.
- The Stats list reads each member with their Persona's name only. Ctrl+Down or the right stick down walks the Persona's stats, skills with costs and affinities line by line. This is the same details list battles have, now outside battle too.
- More lists keep their descriptions for the same details keys instead of reading them after every row: the camp menu's entries, items in the camp menu and in battle, the camp menu's skills, Equip's gear, the skills on a Persona's status page (one just learned still says what it does), the fusion skill-inheritance list and the game's Config. Backslash reads a row with its description in one go, as before. Shops, requests, the calendar, Shuffle Time's cards and event choices still read theirs at once.
- F1 mentions the details keys wherever the focused row has details.
- Choosing who gets an item, casts or receives a skill, or changes gear no longer reads each member's level.
- The mod settings are in five categories instead of one list of 27: Exploring, Sonar, Sound, Dialogue and movies, Controls and help. Tab and Shift+Tab, Page Down and Page Up, or RB and LB change category.
- Mod settings: pressing on at the first or last row says the row again without the cursor sound, as the game's menus do.

### Fixes
- After Shuffle Time, enemies finished by an All-Out Attack are no longer read as healed, down and defeated a second time.
- On a controller, moving to the first or last target on the command wheel sent its line to the screen reader twice.

## 0.1.0-alpha.3

### Sonar (new)
- Every point of interest near you now repeats its own sound from where it is, faster and louder the closer it is: exits a rising breath of air, doors two knocks, people a two-note whistle, shops a small bell, objects a plucked note. On by default; F3 (both triggers and R3 on a controller) turns it on or off.
- It works with the points of interest list: the point you selected is marked by a short tick before its sound and always has a place among the sounds, and each point you land on in the list plays its sound once from where it is, however far.
- New mod settings: Sonar, Sonar plays (everything nearby, the list's category only, or the selected point only at any distance), Sonar range, a switch for each kind (changing one plays its sound), and Sonar volume.
- It reaches 10 m unless you change it (up to 40), and a thing sounds the moment it comes into range.
- Its sounds take turns: one at a time, each after the last has finished, the most overdue first, so they never play over one another however many things are near.
- The nearby chimes now play only while the sonar is off.

### Menus
- What a menu presses in the game for you (stepping the game's highlight, the confirm) now goes out on the device you pressed: a key for the keyboard, a button for the controller. It used to follow whichever device had stirred last, so a controller's stick resting near its edge could turn a keyboard confirm into a controller press.
- Returning to the title from System no longer reads "Menu, System, 9 of 9" again after you confirm.
- Rewind: you can now go back to any day in the list. The game's cursor did not follow the focus, so only the day the list opened on could be confirmed.
- Rewind: the list waits for its opening animation like the other menus, and nothing is read again behind the fade after you confirm a day.
- A prompt such as "Check: Dorm Register" is no longer said behind the black screen while a rewind or a load takes you away.

### Shadows in Tartarus
- The Shadow's growl is heard from 30 m instead of 15, is louder at a distance, and is pitched where headphones carry it. A Shadow above or below you is no longer left out, which could keep one silent until it was on you.
- The Shadow's growl is now a longer, rattling snarl and repeats faster: every 0.8 seconds at 30 m, running together into one unbroken snarl beside you.
- Strike reach: while a Shadow is inside the reach of your strike, a quick ting of steel repeats fast (about six a second, stopping the moment you leave the reach) in the middle of your head (it is about you, so it has no direction), so you know the attack will connect before you swing. It is read from the game's own hit area. Behind the Shadow, the ring is the two quick ticks that mean the strike brings the advantage; those used to rely on an estimate of the distance.
- A Shadow that notices you sounds a harsh two-note alarm from where it is, and its growl is higher, louder and faster while it hunts you. Two soft falling notes mean it has lost you.
- The strike-reach ring also plays for a breakable object while your strike would break it. A Shadow in reach takes the ring first. Like the other Shadow cues it needs the Shadow alerts setting on.

### Exploring
- Pushing the controller's left stick stops the auto-walk, as a movement key already did on the keyboard.
- The auto-walk finds the way in to a thing that stands inside something solid, such as a Tartarus teleporter between its pillars: it checks which sides the hero's body can come in from, walks round to the nearest and goes straight in. It used to circle the thing a sixth of a turn at a time, which could miss every gap and end in "Stuck".
- Exits are said by name alone: "Iwatodai Station", not "Iwatodai Station exit", which was taken for the way out of the place.
- Prompts say "Go to" only before a place an exit leads to. A prompt that already says what you do, like "Enter the Velvet Room", and a device you operate are said by name alone.
- The auto-walk ends when a dialogue or menu comes up, instead of walking on into a way the game has just turned you back from.

### Sounds
- A sound with a place in the world now stays at that place while it plays: the sonar's sounds, the beacon, the Shadow cues and the nearby chimes shift in your ears as you walk or turn the camera during them. Each used to stay where it was heard when it started.
- None of the mod's field sounds play in a battle, a story scene, a conversation, a menu or shop, behind a faded-out screen, or while the mod's settings are open. The wall tones and the nearby chimes used to go on through battles and scenes; the wall tones, the sonar, the chimes, the Shadow cues and the beacon now all follow one rule.
- The mod's plain beeps are replaced by sounds made to match the sonar's. The beacon is a short rising chirp, the Shadow pulse a low, rough growl, the "strike from behind" cue two quick high ticks, and the controller layer's tick is softer.
- A nearby chime is now its kind's sonar sound, played once, instead of a beep at one of five pitches.

### Battles
- The hero's level-up page on the victory screen no longer reads the equipped Persona and its level. Among your new level, HP and SP it sounded as if the Persona had levelled too. A Persona that does level up is still announced on its own line.
- The target no longer wraps around on the command wheel or in Analyze. At the first or last enemy, a further press says who you are still on, so you can tell the leftmost from the rightmost.
- A skill or item that hits a whole side is now one choice in the target list: "All enemies" or "All allies" with their names, instead of a single enemy's name.
- Damage or healing on the one target you chose yourself is said as the number alone: "14 damage", "healed 53". The name stays wherever you did not pick the target: enemies' and allies' own actions, anything that hits several, and an attack sent back at you.
- Enemies finished by an All-Out Attack are announced as defeated when the battle is won, not after Shuffle Time.
- Rush is announced: "Rush" when you turn it on, "Rush off" when you stop it or the game ends it, as a boss's mid-battle lines do. It was silent.
- The word the game shows beside a hit is said: "Weak", "Critical", "Resist", "Miss" or "Block", followed by "on" and the name unless it is the one target you chose. A miss or a blocked attack used to say nothing at all.

### Keys (F1)
- On a Tartarus floor, F1 now also names Auto-Recover (Y, or RB on a controller) and Navi (N, or View).
- F1 on a dialogue choice list names its keys. It said "No keys to tell here".
- The Auto-Recover tutorial names Y on the keyboard, not E.

### Game over
- The poem before the choices is read: "Game Over", then its four lines as the game brings them in. That stretch was silent, apart from your fallen allies' HP being read out, which is gone.
- Confirm during the poem skips the rest of it and goes on to the choices. The game itself only finishes one line per press.
- Each choice is followed by its own description. The wrong one was read first and then corrected, and a description could repeat or follow you to the next choice.
- The choices are announced once the game takes input on them. They were announced a moment early, so a confirm pressed at once was lost.

### Victory screen
- The screen no longer waits for the game's animation. The summary is read at once, your confirm brings the level-ups straight away, and a confirm with nothing left to hear takes you through the rest of the animation to the next page that is yours to read.

### Shuffle Time
- Each card now says what its face shows before its effect: "Fool", or a suit card's suit and number such as "Wands 1".

### Velvet Room
- A Persona's status page in registration and in the compendium is read again. It was silent: the list behind it kept the focus. The page now opens with what confirming does, "Register this Persona?" or "Summon this Persona?", before the Persona's name.

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
