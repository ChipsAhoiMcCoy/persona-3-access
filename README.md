# Persona 3 Access

A screen-reader accessibility mod for Persona 3 Reload on Steam. It speaks the game through your screen reader and adds sound for getting around: wall tones, a sonar of the people, doors, exits and things around you, and an auto-walk to any place of interest.

This is an unofficial fan project, not affiliated with or endorsed by Atlus or SEGA. It includes no game files; you need your own copy of the game. Persona 3 Reload and related names and characters belong to their owners.

## Status

Alpha. Most of the game is covered and much of it tested, but nobody has finished the game with the mod yet. See [KNOWN_ISSUES.md](KNOWN_ISSUES.md), and please report what you find.

The mod is made for the current Steam version of the full game. On anything else (the demo, an older update) it says so when it loads and reads messages and the field but not menus, because their different layout froze the game.

## Requirements

- Persona 3 Reload, Steam version (app 2161700), on Windows 10 or 11.
- A screen reader such as NVDA or JAWS. Without one, the mod uses Windows speech.
- A keyboard. A controller is optional, but typing your name needs a keyboard.
- Headphones are recommended.

## Install and uninstall

1. Download the zip from the Releases page and extract it.
2. Copy `dsound.dll` and the `P3RAccess` folder into the game's `P3R\Binaries\Win64` folder, the one with `P3R.exe` (by default `C:\Program Files (x86)\Steam\steamapps\common\P3R\P3R\Binaries\Win64`).
3. Start your screen reader, then the game from Steam.

To uninstall, delete those two from `Win64`. Settings and logs are in `%LOCALAPPDATA%\P3RAccess`; delete it too if you want them gone. Saves are not touched. Details and troubleshooting are in [INSTALL.md](INSTALL.md).

## First run

The game takes a while to start; you will hear "Persona 3 Access loaded". The caution and photosensitivity notices each stay up until you press Space or A. Then comes the title menu: up and down arrows or the D-pad, Enter or A to choose.

Learn two keys first: F1 says the keys of whatever is in front of you, and F2 opens the mod's settings. On a controller, hold both triggers and press Menu or View.

## Keys

The game's own controls work as usual. The mod adds:

- F1: Keys of what is in front of you (menu, screen, message window or field)
- Shift+F1: The mod's own keys
- F2: Mod settings
- F3: Sonar on or off
- Slash: Where am I: area, date, yen and current objective
- Shift+Slash: Social stats with their ranks
- Shift+Backslash: Social Links: each arcana with its partner and rank
- Backslash: Repeat the current message, or describe the current screen
- Right / Left bracket: Next (farther) / previous (nearer) point of interest in the category
- Shift+Right / Shift+Left bracket: Next / previous category, read from its nearest
- Apostrophe: Walk to the selected point of interest (press again, or move, to stop)
- Shift+Enter: Walk to the selected point and use it: Space is pressed for you once its prompt (check, talk, open) comes up. Nothing is pressed where no prompt comes, as at an exit
- Shift+Apostrophe: Party: each member's level, HP and SP
- Semicolon: Describe the area, or while someone talks, describe them
- Shift+Semicolon: The selected point of interest again, from where you stand, with its sound

### On a controller

Hold both triggers (LT and RT) for the mod's keys. You hear a tick and, in the field, the selected point of interest (or the nearest of the current category if none is selected). While you hold them the game ignores your buttons:

- D-pad left / right: Previous / next category
- D-pad up / down: Previous (nearer) / next (farther) point of interest
- A: Walk to the selected point (again, or push the left stick, to stop)
- B: Describe the area
- X: Repeat the message, or describe the screen
- Y: Where am I
- LB: Social stats
- RB: Social Links
- L3: Party
- R3: Sonar on or off
- Menu: Keys of what is in front of you (like F1)
- View: Mod settings (like F2)

A single trigger reaches the game 0.15 seconds late (adjustable in the settings), so pressing both does not also dash or turn; a quick tap still works.

In top-down mode (on by default), the right stick no longer turns the camera while you walk. Push it toward a direction (up is north) to snap to the nearest point of interest that way: its name and distance are said ("Iwatodai Station, 8 meters"), or "nothing", and its sound repeats alone from where it is while you hold the stick, even with the sonar off. It speaks again when another point takes over. Press RT while on a point to walk there and use it, as Shift+Enter does; that press does not dash. The scan has no keyboard key yet.

### In menus

Every menu (title, camp, shops, the Velvet Room, battle lists and the rest) is the mod's own: it reads each entry from the game's data as you move and moves the game's highlight to match. Use the arrow keys; W, A, S and D do nothing in these menus.

- Up / Down (D-pad): Move through the list, with its position ("3 of 9"); at either end the item is read again
- Left / Right (D-pad): Move along a row, change a value, open or close a group; in a plain list they reach the game
- Home / End: First / last item
- Enter or Space (A): Choose
- Tab / Shift+Tab (RT / LT): On a page in sections (a Persona's status page, a member's Stats page), next / previous section with its title ("Stats, Strength 54, 1 of 5"). Page Down / Page Up do the same, also where Tab is the game's. On a member's Stats page LT and RT stay the game's (Theurgy, Persona status)
- Tab / Shift+Tab: Next / previous panel, on screens with more than one
- Backslash (triggers + X): The focused item in full, with its description
- Ctrl+Down / Ctrl+Up (right stick down / up): Next / previous line of the focused row's details (see below)

Everything else (C to go back, Q and E for pages) is the game's, and if the game moves its highlight (stick, mouse, page change) the mod follows.

**Details.** Lists you go through often read only what you need to choose: name, cost or count, position. The rest is on the details keys, starting from the top each time you land on a row. This covers the camp menu's entries and help lines; items and skills in the camp menu, in battle and in fusion inheritance (what they do); Equip's gear descriptions; skills on a Persona's status page (a skill just learned is still described at once); the Stats list; Config's setting descriptions; and whoever is in focus in battle. Descriptions are still read at once where they are the point: shops, requests, the calendar, Shuffle Time's cards, difficulty, a skill being learned, and choices in events. F1 mentions the details keys wherever a row has them.

### Key hints

Key hints are only spoken when you press F1 (or triggers + Menu): the keys of the current menu, battle, conversation (next, log, auto, skip), tutorial (Q and E, or LB and RB, turn pages; each page is read with its position) or field ("Space checks and talks, Tab menu, M map, ..."). Escape is Back / Cancel in menus and battle prompts, except in the contract, where it is the game's; C, the game's own key, works too (on the battle command wheel both are the game's Guard).

Hints follow what you play with: after a controller button they name Xbox buttons ("A reads, B closes"), after a game key they name keyboard keys again, and tutorial button icons switch the same way. The mod's own keys don't switch them. With a controller connected and nothing pressed yet, hints start on the controller.

In a conversation, Auto (Shift, or X) and Fast-forward (Tab, or Menu) are announced on and off. While Auto is on, the "When a line ends" cue is not played. During fast-forward, lines are not read but still go in the log (B); when it stops, the line on screen is read.

### More keys on particular screens

F1 always gives the current screen's keys. These are not described elsewhere here:

- Stats: F adds or removes a party member.
- Camp Persona: H releases a Persona.
- Quest: X sorts.
- Compendium: Q and E sort, Tab shows registered Personas only, F locks.
- Registration: Tab marks, F registers.
- Fusion Search: Q and E sort.
- Shops: Q and E switch member or category.
- Area map: Tab zooms.
- Battle: Q on the target list analyzes; A and D switch target in Analyze.

## Mod settings

F2 (or triggers + View) opens the settings from anywhere; the game's Config screen lists them too. The game gets none of your keys while they are open, but keeps running: in Tartarus, open them from the camp menu's Config to be safe.

They work like any other menu: up and down choose (at either end a further press repeats the row), Home and End jump, left and right change the value, Space or Enter (A) steps through values, F1 (Menu) reads these keys, and Escape, Backspace or F2 (B or View) closes. Tab and Shift+Tab (Page Down / Page Up, RB / LB) switch category, wrapping. Each row says what it does after its name, value and position. Changes are saved at once to `%LOCALAPPDATA%\P3RAccess\settings.ini`. The menu plays the game's own menu sounds.

**Exploring**

- Wall tones: on, off
- Shadow alerts: on, off. In Tartarus: the Shadow pulse, the alarm when one notices you, the strike-reach ring (for Shadows and breakable objects) and the back-turned / facing-you cues
- Points of interest range: 60 meters; 20, 40, 100 or 150
- Wall tone volume: 100 percent; 0 to 100 in steps of 10
- Top-down exploration: on, off (see below)

**Sonar**

- Sonar: on, off (also F3)
- Sonar plays: everything nearby, the list's category only, or the selected point only (at any distance)
- Sonar range: 10 meters; 5, 15, 20, 30 or 40
- Sonar: exits, doors, people, shops, objects, events, Twilight Fragments: on, off, each kind on its own. Changing one plays its sound
- Sonar volume: 80 percent; 0 to 100 in steps of 10

**Sound**

- Sound placement: stereo, 3D for headphones, plain left and right. How the sonar and Shadow cues are placed. Stereo (Dark Vision's model, the default) places them by level and by the moment each ear hears them, and darkens sounds behind you. 3D uses Steam Audio's HRTF, which also gives above and below. Plain only pans
- Lower pitch behind you: on, off. The sonar and the Shadow pulse play lower from behind (in top-down mode, from the south)
- Mod master volume: 100 percent; 0 to 100 in steps of 10. Every sound the mod plays, not the game's own
- Shadow sounds volume: 60 percent; 0 to 100 in steps of 10. The Shadow cues and the controller layer's tick

**Dialogue and movies**

- Voiced dialogue: not read (the voice plays alone), speaker's name only, read aloud. Unvoiced lines are always read. A voiced line is not read even if its voice is silent (voice volume at zero): choose read aloud then. A speaker is named only on the first of their lines in a row
- When a line ends: say Next, page turn sound, message chime sound, cursor tick sound, off. The cue when a line waits for you to continue: on a voiced line when the voice finishes, on an unvoiced one after the reading (a sound plays as the reading begins on screen readers that can't report its end, such as NVDA)
- Read cutscene subtitles: off, on. They are in the log (B) either way
- Audio description in movies: on, off. A narrator describes the animated story movies between lines of dialogue, dipping the movie's sound
- Audio description volume: 100 percent; 0 to 100 in steps of 10

**Controls and help**

- Controller trigger delay: 150 milliseconds; off, 100 or 250
- Menu key hints on entry: off, on. Menus say their keys after their first line, besides F1
- Mod keys: Space reads the mod's keys

**Sound test** plays each of the mod's sounds with Space or Enter (A), whatever is switched off, and says what it means: each sonar sound (and one played from behind you), the Shadow's growl (plain and hunting), the Shadow alarm and losing you, the two strike-reach rings, the four wall tones and the controller tick.

## What is covered

### Story, messages and movies

- Dialogue, inner thoughts ("Thinking: ..."), system notices and their pages, controller notices ("A device was disconnected"). Subtitles of movies and cutscenes only if turned on in the settings.
- Choices: the highlighted answer and its position, as the list appears and on every move ("Go with this, 1 of 2"). A script that asks you to pay (the late-night TV shopping) says the price and your money first ("Price 2980 yen, you have 4800 yen").
- The log (B, in the field or a conversation): each line as you scroll, opening on the newest; Space replays its voice. Lines from before this session are said to be earlier. Closing it in a conversation reads the line on screen again.
- The party's voiced chatter while exploring, and the navigator's and party's remarks in battle, are not read over the voices; they stay in the log. Unvoiced lines are read.
- Movies: "Movie paused" (Enter pauses and resumes, Tab skips). Every animated story movie (the anime scenes, both openings, both credit rolls) has audio description: a narrator describes what is on screen between lines, never over dialogue. The 3D story scenes are not described yet. During the credits, Backslash reads the names on screen.
- Cut-ins: a close-up flashed to stress a line is described before it ("Close-up: Yukari, glaring.").
- Pictures shown full screen are described as they appear (a class question's chalkboard, a sketch in the story).
- Semicolon (triggers + B) while someone talks with their portrait up describes them as the portrait shows them now: their look, the outfit (uniform, winter coat, swimsuit, yukata, combat gear...) and how they stand. Every outfit of every character with a portrait is described.
- Messages (Ctrl, or the morning's "I've got a text message..."): each mail as sender and title, "new" when unread; Space reads it, Escape closes. The reply choice that follows is read like any choice.
- A new day: the date, weekday, time of day and moon as the day-change screen starts ("April 7, Tuesday, Morning. Waxing gibbous moon"), and a new time of day as its screen plays. A date reached any other way (a load, a story jump) is said once you can walk.
- Game over: "Game Over" and the poem are read whole; Space or Enter (A) skips to the choices, each read with its description.

### Start of the game

- The caution and photosensitivity notices stay up until you press Space or A, instead of moving on after 7 seconds.
- Title screen, title menu and difficulty (with descriptions).
- The contract is a menu ("First name, blank, 1 of 3", "Last name", "Done"). Enter (A) on a name opens it for typing; each letter typed or deleted is said, the left and right arrows, Home and End read the character they land on, up and down read the whole name, and Enter saves. Enter (A) on Done signs. On a controller the letters still come from a keyboard.

### In the field

- The area when you arrive, with the part and floor the game's banner names ("Iwatodai Dorm, Hallway, floor 2"); the objective when it changes ("Objective: Head to the faculty office").
- Interaction prompts ("Check: Vending Machine", "Go to: Iwatodai Station"; prompts that already say what you do, and devices, by name alone). Over a Social Link partner, their arcana and rank ("Talk: Kenji Tomochika, Magician, rank MAX").
- Items and money gained or spent ("Got Bead times 3, 500 yen"), and party HP or SP changing outside battle.
- Describe the area (Semicolon, or triggers + B): a written description of where you stand: every area of the town, the school, the dorm, Yakushima, Kyoto, the full-moon operations, Tartarus and the Abyss of Time. Outdoors it follows the time of day, the season and story changes (the Dark Hour, Christmas, January).
- The town map ("Go Elsewhere") is a numbered list in map order ("Paulownia Mall, 2 of 3"), opening on where you are; the stick still moves around the map and the list follows. Each destination says who is there ("Social Links here: Kenji Tomochika (Magician), rank 1, ready to rank up"). Space picks it, then its entrances (a closed one says so). F lists its shops and places, saying which are unavailable.
- M in town, school and the dorm: "Where should I go?" with the area's destinations and "Town Map" first. Where the story allows no travel, M opens the area map instead (C closes it). Other town keys: H saves anywhere, Ctrl opens messages, B the log.
- SELECT lists (the shrine's fortune and the like): the activity and question, then each entry. The rooftop garden lists each seedling with how many you own and what it grows, in how many days.

### Points of interest

Points of interest come in categories, each listed nearest first, empty ones skipped:

- Exits: ways into another area, named for where they lead ("Gekkoukan High School" there is the way into the school).
- Doors: rooms you check to go in (classrooms, offices, the dorm's bedrooms). A door between two rooms is listed once, by where it leads.
- People: by name. Social Link partners with their arcana ("Yukari Takeba (Lovers)"), or as the camp describes them before you meet ("Jersey-Wearing Student (Chariot)"). After the name, what the icon over their head says: "talked to", "heard", "Social Link", "Social Link, reversed", "hangout", "Linked Episode", "request", "study group", "walk". Nothing after the name means they have something new to say.
- Shops: businesses, food, vendors and vending machines.
- Objects: everything else you can check, including the glowing Twilight Fragments ("Twilight Fragment") and in Tartarus the crystals that give one ("Twilight Fragment crystal"). Picked up or broken ones leave the list.
- Events: spots that start a fight or scene when you walk into them ("Gatekeeper" on a Gatekeeper's floor until it is beaten).
- Shadows, in Tartarus.

A point is read with its name, distance, clock direction (12 o'clock is where pushing forward walks; in top-down mode, the compass) and position: "Iwatodai Station, 6 meters, 2 o'clock, 1 of 3". The category is said first when you change it or open the controller layer. Only the part of the map you are in is listed, on your floor; past a closed door is listed once you go through. Tartarus lists its whole floor.

Auto-walk turns you to face what you walked to, so its prompt appears; stairs and doors end it on the far side. A party member's remark on the way pauses it until you close the remark.

With the sonar on, landing on a point (or Shift+Semicolon) plays its sound alone, repeating at its distance's rhythm from where it is, however far, while the mod's other sounds go quiet. This lasts about 3 seconds (or while you hold the triggers) and ends once you walk.

### Tartarus

- The block and floor ("Thebel, floor 3"); stairs ("Head Up") and the Monad door ("Enter Door") under Exits.
- Shadows by kind ("Shadow", "Strong Shadow", "Rare Shadow", "The Reaper"), with the sound cues under Sounds.
- Chests by kind ("Treasure Chest", "Rare Treasure Chest", "Gold Treasure Chest", "Locked Treasure Chest, 2 Twilight Fragments"); items read as you pick them up; opened chests leave the list.
- Breakable objects; a missing person under People until rescued, named once the police notice shows it ("Missing Person, Ayako Yoshimoto"; walk into them to rescue them).
- Terminals: each floor you can travel to, the furthest marked; Space travels.
- Auto-Recover (Y, or RB) and the entrance clock read the new HP and SP. N (Navi) and the other helpers speak through messages.

### Camp menu

Tab opens and closes it; Escape backs out of a submenu. Each entry reads with its position (the help line is on the details keys); one the story has not opened says "unavailable".

- Item and Skill: items with how many you own; choosing who casts or receives reads each member's HP and SP, then the new HP.
- Equip: in the gear lists F (X) reads the item again with its description.
- Persona: one Persona at a time as its status page, read as lines in sections that Tab and Shift+Tab (RT and LT) jump between: the Persona, arcana, level ("equipped"; Space on this line equips it) and EXP to next level; Stats ("Strength 54"); Skills with costs; Learns later, with levels; Affinities ("Weak to Light", "Repels Fire Wind"). Q and E (LB and RB) switch Persona. The same page is used in the Velvet Room.
- Stats: each member with level, HP, SP and Persona ("Yukari, level 3, HP 74 of 74, SP 61 of 61, 2 of 3, Io, Lovers, level 3"). Space opens their page as lines: the member, your social stats, their Persona, its stats, skills, affinities; Q and E switch member, or to Tactics ("Yukari: Direct Commands"; Space opens it).
- Theurgy: once someone knows one, Z (LT) on a member's page opens the list ("Makoto, Cadenza, 1 of 2" and its effect): left and right choose, Q and E (LB and RB) switch character, F (X) reads what fills their gauge. A Social Link event that grants one says so ("New Theurgy: Cyclone Arrow. ...").
- Social Links: the list says when a link is reversed or ready to rank up. Space opens its page on the link and its latest memo, then each person's profile; Q and E switch link.
- Quest: Elizabeth's requests (from May 10, also at her counter): title, then task; F reads the full request and reward; Space accepts. Missing Persons (E): each case with its number, name once known, deadline and status, and the profile of each person identified.
- Calendar: left and right move a day, up and down a week, each day read with its weekday and plans; Q and E change month; F opens the part-time jobs, each with its time, pay, place and whether that day takes it.
- System: Config (each page, then each setting with its value and position, "Vibration: on, 1 of 24"; left and right change it; Space saves, H restores defaults; Key Configuration reads each binding; BGM rows open a playlist, Space sets a track, F plays it), Dictionary, Tutorial (tabs Q and E switch; button icons read as key names), Save and load, and Rewind (each day you can go back to).
- Save and load: each slot with name, date, time of day, level, location and play time; a clear save says so ("Slot 9: Makoto Yuki, Clear data, start a new game, play time 46 hours 16 minutes").
- Learning a skill (a skill card, or a level up): the skill, then the Persona with its stats and skills; Q and E switch Persona, Space learns. When the eight slots are full the arrows move over them (two columns of four), each read with its cost; Space forgets it, Escape goes back. If a skill is about to change, each change is said ("Bufu would become Spirit Drain").

### Social stats and Social Links

- "Stats up" screens: each stat that rises, and rank ups with the game's rank names.
- A new link in two parts: "New Social Link" as the poem appears, then the link and rank once the card turns ("Strength Social Link, Yuko Nishiwaki, rank 1"). A rank up with no poem says it in one line.
- Points earned in a conversation: "Bond deepened: Magician, 3 points", with "ready to rank up" when the next rank is reached.

### Battles

Each turn uses the game's own keys (F1 names those open this turn). A turn opens with whose turn it is with HP and SP, then the target with its health, buffs, debuffs and the affinities you have found ("Junpei, HP 99, SP 17. Cowardly Maya, 35%, Down, weak to Slash, Fire"); unavailable commands are named. Moving the target adds its place ("1 of 2"); targets do not wrap. Backslash repeats the summary.

- Keys: Space attack, E skills, R items, C guard (asks first), Q analyze, W tactics, Tab rush, A and D or left and right change target, S survey (up and down go through everyone), F assist, T Theurgy when "Theurgy ready" is announced.
- Skill lists read name and cost (a physical skill's cost in HP); Space confirms, Q and E change Persona, F shows its status page. A skill or item that hits a whole side asks once ("Use Mazio Gem on all enemies? Space to confirm, Escape to cancel.").
- What enemies and members acting on their own used, damage, healing and defeats ("2 Cowardly Maya defeated"); on a target you chose, the number alone ("14 damage"). Your own choices are not read back.
- Details of whoever is in focus (Ctrl+Down, right stick): name and level, HP, SP, ailments and buffs with turns left, guarding, affinities found, and for a party member their Persona, stats and Theurgy gauge. In Analyze also maximum HP and SP, arcana, attack type, skills and drops as far as the game shows.
- "S.E.E.S. advantage" or "Shadow advantage" at the start; "1 More! Shift available", after which B (Shift) passes the turn; the All-Out Attack prompt once with its buttons.
- Tactics (W): "Give an order?", each member's order, and the orders. Rush (Tab) lets the party fight alone.
- A battle against the clock (the Priestess on the monorail) says the time left at the start, with each of your turns, every 5 minutes, at 1 minute, 30 and 10 seconds, and when the story cuts it short.

### After battles

- Shuffle Time reads every card's effect as they are dealt, with its position, and says when you may take more than one ("choose 1 more"). A skill card says how many you own; a Persona card's status page is read. Arcana Burst's row reads the same ("Arcana Burst. choose 2"; C cancels the last pick).
- Persona Overstock: the new Persona ("Gain: ...") and each one you hold; Space releases the one chosen, F shows its stats.
- The victory screen reads each thing as its own line you can go back over with the arrows: EXP, money, items, each level up, the hero's level-up page, and the status page of a Persona that learned a skill. Space or Enter goes on.

### Velvet Room

- Its menu (Fusion, Summon from the Compendium, Register to the Compendium) and the fusion method.
- Dyad Fusion: each Persona with arcana and level; on the second list each row also says what the pair makes ("Apsaras, Priestess, level 2, makes Nekomata, Magician, level 3") or "cannot fuse", and whether it is above your level or already in stock. Right and Left step through the result's details.
- The result page is the status page, with the Social Link bonus ("Social Link bonus: rank 8, 32793 EXP, up to level 28"); Q and E page to the materials.
- Inheritance: "Choose skills to inherit, up to 2.", each skill with its cost, and "chosen, 1 of 2 picks" as you choose; Igor's "Is this Persona acceptable?" adds what it inherits. "Fusing." marks the animation (Tab skips). The bonus level up is a page too ("Agility 21, up 7", new skills marked "new").
- Fusion Search names each Persona your stock can make and from which two; Special Fusion lists recipes with materials and how many you have.
- The compendium reads registered Personas with their price and says which are in your stock; Registration lists your Personas, then their status page.

### Shops

- Weapon and item shops: items with price and stats, descriptions, how many you own, why a row can't be bought ("not enough yen", "out of stock"), "How many?" with the total as you change it (W and S), and the buy and equip prompts.
- Mayoido Antiques (from July): Fuse Equipment, Trade Card, Trade Item, each entry with how many you own, its stats and what the trade needs ("needs Topaz 3, you have 1").
- Vending machines say how many are left. Be Blue V's job is read as choices.

## Top-down exploration

The game's camera sits behind the hero and turns, so a sound "ahead" kept changing direction. Top-down mode (on by default) presents the field the way Dark Vision does:

- While you walk freely the camera faces north and cannot turn. Up always walks north, even where the game uses a fixed scene camera (Fountain Square, the station, many rooms).
- The game's sounds and the mod's are heard from where you stand with north ahead, as if from above.
- North is the corner map's up, place by place: where the game draws a map turned (Port Island Station, your room, the Command Room), north turns with it.
- The wall tones are a compass: north, east, south and west, reaching 3 m. While you move, the direction behind your walk stays silent.
- Points of interest give compass directions ("north-east"), and the right stick scans (see On a controller).

Conversations, story scenes, menus and battles keep the game's own camera and sound. Turn the setting off to play with the game's camera.

## Sounds

- **Wall tones:** four looping tones for how close walls are: ahead and behind (centered, different pitches), right and left. Louder is closer; silence is open space. Each measures how far you could actually go that way, so a wall you would slide along stays quiet.
- **Sonar:** every point of interest in range (10 m by default) repeats its own sound from where it is, faster and louder as you get closer: about once a second at 10 m, nearly twice at 5 m, nearly seven times at 1 m. A thing is heard from its part nearest you, so a long counter or wide exit sounds from beside you; inside its area, from its middle at the quickest rhythm. Sounds play whole, overlapping when fast. Exits, objects, events and Twilight Fragments use Grim Dark's sounds (its dungeon entrance, interactable, ruined shrine and loot). Doors are a hollow knock, people a whistled "yoo-hoo", shops a till's ching. Directions follow the camera (or north, in top-down mode). To keep crowds readable, at most four things sound at once, two of a kind; the selected point always has a place, and anything within 2.5 m makes the rest quieter. Things on another floor or past a closed door are left out until you can reach them. The sonar is silent in menus, conversations, story scenes and battles.
- **Shadow pulse:** in Tartarus, the nearest Shadow within 30 m sounds Grim Dark's enemy ping from where it is, from every 0.8 seconds at 30 m to every 0.2 seconds beside you. When one notices you an alarm sounds from it and its pulse grows urgent while it hunts you; a soft falling note means it lost you. Within 4 m you hear whether it has its back turned or is facing you.
- **Strike reach:** while a Shadow is inside your strike's reach (the game's own hit area, counted a little short), a quick steel ring repeats in the middle of your head: attack now and it connects. When you are also behind it, a glassy upward tick plays instead: the strike gives the advantage. A breakable object rings the same way. Part of Shadow alerts.

Placed sounds stay where they are while they play, so they shift as you move. Positioning is set by the Sound placement setting. The 3D option uses Steam Audio (Valve, Apache 2.0, `phonon.dll`); without that file it falls back to stereo.

## Known limits

The 3D story scenes have no audio description, only the animated movies. A game update can break the mod. The full list, including what has not been tried yet, is in [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

## Reporting bugs

Please open an issue at https://github.com/ChipsAhoiMcCoy/persona-3-access/issues with:

- What you did, what you heard, and what you expected.
- Your screen reader, and keyboard or controller.
- The log, `%LOCALAPPDATA%\P3RAccess\p3access.log` (after a restart, the previous session is `p3access.last-play.log`). It holds what the mod spoke, including story text, and may show your Windows user name, so look it over before sharing.

## Credits

Persona 3 Access builds on Grim Dark by Austin Hicks and Dark Vision by Bradley Renshaw. It uses Prism, Steam Audio, Microsoft Detours and miniaudio. The movie narrator is the Kokoro voice model's af_heart voice. Thanks to the Persona 3 Reload modding community. See [CREDITS.md](CREDITS.md) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License

The mod's code is under the zlib license ([LICENSE](LICENSE)). Parts from other projects keep their own licenses, listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The license covers the mod only, not the game.
