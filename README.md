# Persona 3 Access

A screen-reader accessibility mod for Persona 3 Reload on Steam. It speaks the game through your screen reader and adds sound for getting around: wall tones, a sonar of the people, doors, exits and things around you, and an auto-walk to any place of interest.

This is an unofficial fan project. It is not affiliated with or endorsed by Atlus or SEGA. It includes no game files, and you need your own copy of the game. Persona 3 Reload and related names and characters belong to their owners.

## Status

Alpha. Most of the game is covered and much of it has been tested, but nobody has finished the game with this mod yet. Please read [KNOWN_ISSUES.md](KNOWN_ISSUES.md), and please report what you find.

The mod is made for the current Steam version of the full game. On anything else, such as the demo or an older update, it says so when it loads and reads messages and the field but not menus, because the game's menus are laid out differently there and reading them froze the game.

## Requirements

- Persona 3 Reload, the Steam version, app 2161700.
- Windows 10 or 11.
- A screen reader: NVDA or JAWS. Without one, the mod uses Windows speech.
- A keyboard. A controller is optional, except that typing your name needs a keyboard.
- Headphones are recommended for the 3D sound.

## Install

1. Download the zip from the Releases page and extract it.
2. Copy `dsound.dll` and the `P3RAccess` folder into the game's `P3R\Binaries\Win64` folder. In a default Steam install that is `C:\Program Files (x86)\Steam\steamapps\common\P3R\P3R\Binaries\Win64`. It is the folder with `P3R.exe` in it.
3. Start your screen reader, then start Persona 3 Reload from Steam as usual.

Exact paths and troubleshooting are in [INSTALL.md](INSTALL.md).

## Uninstall

Delete `dsound.dll` and the `P3RAccess` folder from the game's `P3R\Binaries\Win64` folder. Your settings and log are in `%LOCALAPPDATA%\P3RAccess`. Delete that folder too if you want them gone. Your game saves are not touched.

## First run

When the mod is ready you will hear "Persona 3 Access loaded". The game takes a while to get there.

The caution and photosensitivity notices appear first, and each stays up until you press Space or A. Then come the title screen and its menu. Use the up and down arrows or the D-pad, and Enter or A to choose.

Two keys are worth learning first: F1 tells you the keys of whatever is in front of you, and F2 opens the mod's settings. On a controller, hold both triggers and press Menu or View for the same.

## Keys

The game's own controls work as usual (keyboard or controller). The mod adds these keys:

- F1: The keys of what is in front of you: the current menu or screen, the message window, or the field
- Shift+F1: The mod's own keys
- F2: Mod settings (see below)
- F3: Sonar on or off: the sounds of the things around you (see Sounds)
- Slash: Where am I: the area, the date, your yen, and the current objective (with the line under it, if any)
- Shift+Slash: Social stats: Academics, Charm and Courage with their ranks
- Shift+Backslash: Social Links: each arcana with its partner and rank
- Backslash: Repeat the current message, or describe the current screen (elsewhere it says there is nothing to read)
- Right bracket / Left bracket: Next (farther) / previous (nearer) point of interest in the category
- Shift+Right bracket / Shift+Left bracket: Next / previous category of points of interest, read from its nearest
- Apostrophe: Walk to the selected point of interest (press again, or any movement key, to stop)
- Shift+Apostrophe: Party: each member's level, HP and SP
- Semicolon: Describe the area: how the place around you looks, or while someone talks, how they look (see below)

### On a controller

Hold both triggers (LT and RT) for the mod's keys. The moment both are down you hear a tick and, in the field, the
nearest point of interest of the current category. While you hold them the game ignores the buttons you press:

- D-pad left / right: Previous / next category of points of interest, read from its nearest
- D-pad up / down: Previous (nearer) / next (farther) point of interest in the category
- A: Walk to the selected point of interest (again, or push the left stick, to stop)
- B: Describe the area: how the place around you looks
- X: Repeat the current message, or describe the current screen
- Y: Where am I
- LB: Social stats
- RB: Social Links
- L3 (press the left stick): Party
- R3 (press the right stick): Sonar on or off

Without the triggers, while you walk the field in the top-down mode (on by default), the right stick no longer turns
the camera: push it toward a direction (up is north) to hear the nearest point of interest that way, with its
distance, and how far the wall is ("north-east: Iwatodai Station, 8 meters. wall 3 meters"). That point's sound plays
from where it is. Each new direction is read at once.
- Menu button: The keys of what is in front of you (like F1)
- View button: Mod settings (like F2); its last row reads the mod's keys

A single trigger reaches the game a moment late (0.15 seconds; the mod settings can change it), so pressing both does
not also dash or shift the turn; a quick tap still works.

### In menus

Every menu (title, camp and its submenus, shops, the Velvet Room, battle lists and the rest) is the mod's own: it
reads each entry from the game's data, speaks it the moment you move, and moves the game's highlight to match, so
what is on screen follows what you hear. Move with the arrow keys: W, A, S and D do nothing in these menus.

- Up / Down arrows (D-pad): Move through the list, with its position ("3 of 9"); at either end the item is read again
- Left / Right arrows (D-pad): Move along a row (targets, tabs), change a value, open or close a group; in a plain list they reach the game
- Home / End: First / last item
- Enter or Space (A): Choose the item: the game confirms it once its highlight is there
- Tab / Shift+Tab (RT / LT): On a page in sections (a Persona's status page, a member's Stats page): next / previous section, with its title ("Stats, Strength 54, 1 of 5"); the arrows still walk every line. Page Down / Page Up do the same, also where Tab is the game's (registration's registered stats). A trigger jumps as it is pressed (after 50 ms, the time the other trigger has to join it for the mod layer); on these pages the game does not get it, except a member's Stats page, where LT (Theurgy) and RT (Persona status) stay the game's and the D-pad walks the lines
- Tab / Shift+Tab: Next / previous panel, on screens with more than one (elsewhere Tab is the game's)
- Backslash (triggers held + X): The focused item in full, with its description

Everything else (C to go back, Q and E for pages) is the game's, and if the game moves its own
highlight (the stick, the mouse, a page change) the mod follows it.

### Mod settings

F2 (or both triggers and View) opens the mod's settings from anywhere; the game's own Config screen lists it with
its keys (F1 there). While the settings are open the game gets none of your keys or buttons, but it keeps running:
in Tartarus, open them from the camp menu's Config to be safe. The settings are in five categories, each a short
list of its own: Exploring, Sonar, Sound, Dialogue and movies, Controls and help. A sixth, Sound test, plays the
mod's sounds (below). Every row says what it does after its name, value and position, with no further key. Tab and Shift+Tab (Page Down and
Page Up; RB and LB) go to the next and previous category, wrapping, and its name is said as you land. It works like
every other menu: the up and down arrows
(D-pad) choose (at a category's first or last row a further press says the row again, without the cursor sound), Home and End jump to the first and last row, typing letters jumps to a setting in the category by name, left
and right change the value, Space or Enter (A) steps through the values, F1 (Menu) reads these keys, Escape,
Backspace or F2 (B or View) closes. Changes are saved at
once, to `%LOCALAPPDATA%\P3RAccess\settings.ini`. The menu plays the game's own menu sounds: opening and closing
(the camp menu's), the cursor on each move, and a sound on each value change.

Exploring holds Wall tones, Shadow alerts, Points of interest range, Wall tone volume and Top-down exploration.
Sonar holds everything named Sonar and Sonar rhythm. Sound holds Sound placement, Lower pitch behind you, Mod master volume
and Shadow sounds volume. Dialogue and movies holds Voiced dialogue, When a line ends, Read cutscene subtitles and
the two Audio description settings. Controls and help holds Controller trigger delay, Menu key hints on entry and
Mod keys.

Sound test is where to learn the mod's sounds. Space or Enter (A) on a row plays its sound, wherever you are and
whatever is switched off, and the row says what the sound means. It holds the seven sonar sounds, one of them from behind you,
the Shadow's growl (plain and hunting you), the Shadow alarm and the sound of a Shadow losing you, the two
strike-reach rings, the four wall tones and the controller layer's tick. They play at the volumes set under Sound.

- Wall tones: on, off
- Sonar: on, off (also F3): every point of interest near you repeats its own sound from where it is, faster and louder the closer it is (see Sounds)
- Sonar plays: everything nearby, the list's category only, the selected point only (at any distance): narrows the sonar to the category the points of interest list is on, or to the one point you selected there, which then sounds however far away it is
- Sonar range: 10 meters; 5, 15, 20, 30 or 40
- Sonar: exits, Sonar: doors, Sonar: people, Sonar: shops, Sonar: objects, Sonar: events, Sonar: Twilight Fragments: on, off: each kind of thing on its own. Changing one plays its sound, so this is also where to learn them
- Shadow alerts: on, off: in Tartarus, the Shadow pulse, the alarm when one notices you, the strike-reach ring (for Shadows and breakable objects) and the back-turned / facing-you cues
- Sound placement: stereo, 3D for headphones, plain left and right: how the sonar and the Shadow cues are placed. Stereo (Dark Vision's model) places them left and right by level and by the moment each ear hears them, and makes a sound behind you darker. 3D goes through Steam Audio's HRTF, which also gives above and below (far sounds are also a little duller). Plain pans them only
- Lower pitch behind you: on, off: the sonar's sounds and the Shadow pulse play lower when their source is behind you. Not in the top-down mode, where behind is south and sounds darker instead
- Points of interest range: 60 meters; 20, 40, 100 or 150
- Mod master volume: 100 percent; 0 to 100 in steps of 10, like the game's volume sliders: every sound the mod plays (not the game's own sounds, which follow the game's settings)
- Sonar volume: 80 percent; 0 to 100 in steps of 10: the sonar's sounds (changing it plays one)
- Shadow sounds volume: 60 percent; 0 to 100 in steps of 10: the Shadow cues and the controller layer's tick
- Wall tone volume: 100 percent; 0 to 100 in steps of 10: the wall tones
- Controller trigger delay: 150 milliseconds; off, 100 or 250
- Menu key hints on entry: off, on: menus say their keys after their first line, as they used to, besides F1
- Voiced dialogue: not read (the voice plays alone), speaker's name only, read aloud: a line the game voices; unvoiced lines are read at once. A voiced line is not read even when its voice does not play (voice volume at zero): choose read aloud then (backslash repeats any line, B's log has them all). A speaker is named only on their first line in a row: Junpei, then Junpei again, is "Junpei: ..." and then the line alone; Yukari after him is named again
- When a line ends: say Next, page turn sound, message chime sound, cursor tick sound, off: once a line of dialogue or a thought waits for you to continue, the screen reader says "Next" or a sound from the game's own menus plays (changing it plays the sound). On a voiced line this comes when the voice finishes; on an unvoiced one "Next" follows the reading. A sound waits for the reading too when your speech can report its end; on those that cannot, such as NVDA, it plays as the reading begins
- Read cutscene subtitles: off, on: the screen reader reads the subtitles of movies and cutscenes as they appear. They are in the log (B) either way
- Top-down exploration: on, off: while you walk freely, the camera stays facing north and everything is heard as if from above (see Top-down exploration below)
- Audio description in movies: on, off: a narrator describes what is on screen in the animated story movies, between the lines of dialogue (the movie's sound dips while she speaks)
- Audio description volume: 100 percent; 0 to 100 in steps of 10: the movie narrator
- Mod keys: Space reads the list of the mod's keys

### Details (the right stick, or Ctrl+Up and Ctrl+Down)

Lists you go through often read only what you need to choose: the name, the cost or count, the position. The rest
is one flick away. The right stick down, or Ctrl+Down, reads the next line of the focused row's details; the stick
up, or Ctrl+Up, goes back. Landing on another row starts at its top again, which is the row itself. Backslash still
reads the row and its description in one go. F1 mentions these keys wherever a row has details. This covers:

- The camp menu's entries ("Item", then "View or use items").
- Items, in the camp menu and in battle: what the item does.
- Skills, in the camp menu, in battle and in the fusion skill-inheritance list: what the skill does. In battle the
  acting member's details follow it.
- Equip's gear lists: the item's description (its stats stay on the row).
- A Persona's status page: what each skill it knows or learns later does. A skill it has just learned still says so
  at once.
- The Stats list: the member's Persona, its stats, skills with costs and affinities.
- The game's Config: what the setting does.
- Battles: whoever is in focus (see Battles below).

Descriptions are still read at once where they are the point of the row: shops, requests, the calendar, Shuffle
Time's cards, difficulty, a skill being learned, and choices in events.

### Key hints

The mod does not read key hints by itself: press F1 (or both triggers and the Menu button) to hear the keys of the
current menu, battle, message window or the field. Escape is Back / Cancel in menus and on the battle prompts, except the contract, where Escape is the game's own
(the game's own key for it, C, works too; on the battle command wheel both are the game's Guard). Key hints ("Up and down arrows choose, Space reads, Escape closes") follow what you
play with: once you press a controller button
they name the controller's buttons with Xbox names ("Up and down choose, A reads, B closes"; LB, RB, LT, RT,
Menu button), and once you press a key the game uses they name keyboard keys again. Button icons in tutorials
switch the same way. The mod's own keyboard keys do not switch the hints. With a controller
connected and nothing pressed yet, the hints start on the controller.

In the field F1 gives the field keys ("Space checks and talks, Tab menu, M map, ..."); in a conversation the
conversation keys (next, log, auto, skip); in a tutorial its page keys (Q and E, or LB and RB, turn the pages;
each page is read with its position, including one you go back to); in menus and battles their own keys. Turning the game's
Auto on or off in a conversation (Shift, or X) is announced ("Auto on", "Auto off"); while Auto is on, lines go on by
themselves, so the "When a line ends" cue ("Next" or its sound) is not played. Fast-forward in a story scene (Tab, or
Menu) is announced the same way ("Fast-forward on", "Fast-forward off"); while it runs, the lines rushing past are not
read (each would cut off the last) but still go in the log (B), and when it stops, at a choice or by your key, the
line on screen is read.

Points of interest come in categories: exits (each leads to another area and is named for where it goes:
"Gekkoukan High School" there is the way into the school, not out of it), doors (rooms you check to go in: classrooms,
offices, restrooms, the dorm's bedrooms), people, shops (businesses, food, vendors and vending machines), objects
(everything else you can check; among them the glowing blue Twilight Fragments lying around town and in the
underground base of the August full moon, which the game
labels "Something Shining" and the list calls "Twilight Fragment", and in Tartarus the crystals you strike to get
one, "Twilight Fragment crystal"; one that is picked up or broken leaves the list), events (spots that start a fight or a scene when you walk into them, with nothing
to press: a Tartarus Gatekeeper's floor lists "Gatekeeper" until it is beaten) and, in Tartarus, Shadows. Each
category lists its points nearest first, and a category with nothing in it is skipped. The sonar has one sound for
each of the first six, and one more for Twilight Fragments
(see Sounds). Points are spoken with their name, distance, clock direction and position: "Iwatodai Station, 6 meters, 2 o'clock, 1 of 3". The category is said first when you change category or open the controller layer.
12 o'clock is straight ahead (where pushing forward walks), 3 o'clock is to your right.
With the sonar on, each point you land on in the list also plays its kind's sound once from where it is, however far.

### More keys in particular screens

F1 always gives the current screen's keys. These are in its hints but not described elsewhere in this file:

- Stats: F adds or removes a member from the party.
- Camp Persona: H releases a Persona.
- Quest: X sorts.
- Compendium: Q and E sort, Tab shows registered Personas only, F locks.
- Registration: Tab marks, F registers.
- Fusion Search: Q and E sort.
- Shops: Q and E switch member or category.
- Area map: Tab zooms.
- Battle: Q on the target list analyzes; A and D switch target in Analyze.

## What is covered

- Dialogue, inner thoughts ("Thinking: ..."), system notices and their pages. Movie and cutscene subtitles only if you turn them on in the mod settings (F2).
- The log (B, in the field or in a conversation): each line as you scroll (up and down arrows), oldest first, opening on the newest; Space replays its
  voice. Lines from before you started the game this session are not known to the mod and are said to be earlier.
  Closing it in a conversation reads the line on screen again.
- Choices: the highlighted answer with its position as the list appears and on every move ("Go with this, 1 of 2").
- The caution and photosensitivity notices at startup: each stays up until you press a button (Space, or A), instead
  of moving on by itself after 7 seconds.
- Title screen, title menu, difficulty selection (with descriptions), the contract (name entry, with typing echo):
  a menu from the start (up and down arrows or the D-pad: "First name, blank, 1 of 3", "Last name, blank, 2 of 3",
  "Done"). Enter (A) on a name opens it for typing ("Last name. Type it, then press Enter."), Enter again saves it and
  you are back on that name; Enter (A) on Done signs the contract. While typing, each letter you type or delete is said,
  and a name filled in all at once is read whole; the left and right arrows, Home and End read the character they land
  on, and up and down read the whole name. On a controller the letters still come from a keyboard.
- Controller and system notices ("A device was disconnected").
- In the field: the area name when you arrive, with the part of it the game's banner names and its floor ("Iwatodai
  Dorm, Hallway, floor 2"), the story objective when it changes ("Objective: Head to the faculty office"), the
  interaction prompt when you can use something ("Check: Vending Machine", "Go to: Iwatodai Station"; a prompt that already says what you do, like "Head to the Dorm" or "Enter the Velvet Room", and a device you operate are said by name alone; over a Social Link partner also their
  arcana and rank, "Talk: Kenji Tomochika, Magician, rank MAX"), item notices with every item and its count ("Got
  Bead times 3, 500 yen"), wall tones, points of interest (people by name) and auto-walk. The points of interest are those of the part of the map you are in: what lies past a closed door is listed once you have gone through its exit.
- Describe the area (Semicolon, or both triggers and B): a written description of where you stand, as the
  game shows it now: every area of the town, the school, the dorm (each floor), Yakushima, Kyoto, the full-moon
  operations, Tartarus (each block) and Episode Aigis' Abyss of Time. Outdoors it follows the time of day and the
  season (cherry blossoms in April, autumn leaves, bare winter trees); the Dark Hour, Christmas in Paulownia Mall and
  the dorm's Christmas tree, and January's fading colours, litter and cult posters each change it too.
  While someone talks to you with their portrait beside the line, the same key describes that character instead,
  as the portrait shows them now: their look, the outfit they wear (school uniform, winter coat, swimsuit, yukata,
  combat gear...) and how they stand. Every outfit of every character with a portrait is described.
- Points of interest list what you can walk to on your own floor; Tartarus lists its whole floor. Auto-walk turns you to face what you walked to, so its prompt appears;
  stairs and doors end the walk on the far side. A party member's remark on the way pauses the walk; it carries on
  once you close the remark. Social Link partners are listed by name and arcana ("Yukari Takeba (Lovers)"), or as the
  camp describes them before you meet ("Jersey-Wearing Student (Chariot)"). A door between two rooms is listed once,
  by where it leads.
- The town map ("Go Elsewhere") reads as a numbered list ("Paulownia Mall, 2 of 3"): the arrow keys (D-pad) go
  through the open destinations in the order they appear on the map, top row first, left to right. The stick
  still moves around the map itself, and the list follows. W A S D do nothing here. Enter or Space picks it, then its list of
  entrances (an entrance closed now says so). The list opens on the place you are in. After a destination's
  name comes who is there now, as the map's card icons show it ("Social Links here: Kenji Tomochika (Magician),
  rank 1, ready to rank up"; a link you can start but have not is listed under the name the game gives it). F (town info) lists the destination's shops and places, saying which
  are unavailable now.
- The map key (M) in town, school and the dorm: "Where should I go?" with the area's own destinations (your room,
  the lobby, other floors) and "Town Map" first; the up and down arrows choose, Space goes. Where the story allows no travel, M
  opens only the area map instead: it is announced, and C closes it. Other game keys in town: H saves
  anywhere, Ctrl opens messages, B the log.
- Game over: "Game Over" and the poem are read whole as the poem starts to appear. Space or Enter (A) skips the rest
  of its animation and goes on to the choices; each choice is read with its description.
- SELECT lists (who to think of at the shrine's fortune, and the like): the activity and the question, then each
  entry as you move. The rooftop garden's planting list reads each seedling with how many you own and what it
  grows (and in how many days).
- Messages (Ctrl, or the morning's "I've got a text message..."): each mail as sender and title, "new" when unread;
  the up and down arrows choose, Space reads it (sender, title, the whole text), Escape closes. The reply choice that follows (an
  invitation: "Meet with Tomochika, Not now") is read like any other choice.
- The camp menu (Tab opens and closes it; Escape backs out of a submenu): the item under the cursor and its help line;
  an entry the story has not opened yet says "unavailable".
- Movies: "Movie paused" when a movie is paused (Enter pauses and resumes, Tab skips). Every animated story movie
  (the anime scenes, both openings, both credit rolls) has audio description: a narrator voice describes what is on
  screen in the pauses between lines, never over dialogue, and pauses with the movie. Turn it off or set its volume
  in the mod settings (F2). The 3D story scenes are not described yet.
- Cut-ins: when a story scene flashes a close-up of a character to stress a line, it is described before that line
  ("Close-up: Yukari, glaring."), and the line waits for it.
- Pictures: a picture a scene shows full screen is described as it appears (a class question's chalkboard, such as
  the four spirals labelled A to D you choose between, and a sketch in the story); the line after it waits for it.
- Save and load screens: slot, date, level and play time.
- A new day: as the day-change screen starts (after you sleep, or when the story moves on a day), the date, weekday,
  time of day and the moon ("April 7, Tuesday, Morning. Waxing gibbous moon"). A new time of day as its screen plays
  ("Evening"). A date or time of day reached any other way (a load, a story jump) is said once you can walk again.
- Social stats: "Stats up" screens, each stat that rises, and rank ups with the game's rank names.
- Social Links: a new link is read in two parts, even in the middle of the conversation. As the poem appears you
  hear what happened ("New Social Link"; the game's voice reads the poem); after you press OK and the arcana card
  turns in, the link and its rank ("Strength Social Link, Yuko Nishiwaki, rank 1"). A rank up with no poem says both
  in one line ("Social Link rank up: Chariot Social Link, Kazushi Miyamoto, rank 2"). Space continues. After an answer that earns points (the notes over your partner's
  head) you hear "Bond deepened: Magician, 3 points", with "ready to rank up" once the link has the points its next
  rank needs (the next hangout ranks it up). Shift+Backslash and the camp's Social Link list say when a link is
  reversed or ready to rank up. Space on a link opens its page on its first line, the link with its latest memo, then each person
  in it with their profile; Q and E switch link (the new page opens on its first line too).
- Battles: each turn is played with the game's own keys, as the game has it (no command list; F1 names the keys open
  this turn). A turn opens with a short line: whose turn it is with HP and SP, then the target with its health, its
  buffs and debuffs ("attack up", "agility down") and the affinities you have discovered ("Junpei, HP 99, SP 17.
  Cowardly Maya, 35%, Down, weak to Slash, Fire"). Moving the target adds its place ("1 of 2", so two of the same
  Shadow are told apart). The skill list reads each skill's name and cost; what it does is the second line of its
  details (Ctrl+Down or the right stick down, see below). Items with counts, target choice, what enemies
  and members acting of their own accord used, a counter included (a skill, item or Guard you picked yourself is not
  read back, for any member you control), damage, healing and defeats (on the one target you chose yourself the number alone, "14 damage"; with the name everywhere else). Battle keys: Space attack, E skills, R items, C guard, Q analyze, W tactics,
  Tab rush, A and D or the left and right arrows change target (the target never wraps around: at the first or last enemy a further press says who you are still on; the same in Analyze and in target lists; a skill or item that hits a whole side asks once and says the buttons, "Use Mazio Gem on all enemies? Space to confirm, Escape to cancel.", and each hit then says who it landed on), S survey (the up and down arrows go through everyone in the battle), F assist (the navigator's advice). Anyone in focus says their ailments with their health ("Jin, 96%, Down, Poisoned"). The details of who is in focus (the target on the command wheel or a target list, whoever Survey, Shift or Tactics is on, the acting member in their items; in the skill list the skill, what it does, then the acting member): they start at the top each time you land on someone, and Ctrl+Down or the right stick down moves down through them line by line, Ctrl+Up or the stick up back toward the top: name and level, HP, SP, each ailment with the turns it can still last, each buff and debuff with its turns left, guarding, the affinities you have found, and for a party member their Persona, stats and Theurgy gauge. After a 1 More, B (Shift) passes the turn: each member is read with HP and SP. C asks "Guard?" before guarding and says the buttons (Space to guard, Escape to cancel), and when a member's Theurgy gauge is full the turn summary says "Theurgy ready, T": T opens the Theurgy list (left and right arrows
  choose, each read with its effect, Space uses it); in skill lists Space confirms, Q and E change Persona, F shows the Persona's status page, Escape goes back.
  A battle opens with who has the advantage ("S.E.E.S. advantage" when you struck first, "Shadow advantage" when you were caught). In Analyze the details also read the enemy's maximum HP and SP, arcana, attack type, skills and drops, as far as the game shows them.
  Backslash repeats the turn summary. The navigator's and the party's remarks during a battle ("Two targets! You can
  handle them!") are not read, so they never cut off the battle's own news; they stay in the log (B, after the battle).
  The same goes for the party's voiced chatter while you explore: the game's voices play, the mod does not read it over
  them, the Reaper's warning included. A line with no voice is still read.
- After battles: Shuffle Time reads every card's effect as soon as the cards are dealt, each with its position ("1 of
  4", "1 of 1"), and says when you may take more than one; a card you took leaves the row ("choose 1 more" when
  there are picks left), a skill card says how many you own, and a Persona card's status page is read too. Arcana
  Burst's bonus row is read the same way ("Arcana Burst. choose 2"; Escape cancels the last pick). With a full Persona
  stock, the Persona Overstock list follows: the new Persona ("Gain: ...") and each Persona you hold; Up and Down
  choose, Space releases the one chosen (the game asks to confirm), F shows its stats, and Q and E there move
  through the list. The victory screen reads each thing as its own line, after the last blow's news, and you can go
  back through them with the arrows: the EXP, the money, each item found, then each member's and Persona's level up as
  it slides in (these never move you off the line you are on), the hero's own level-up page (HP, SP, EXP to the next
  level; read again after a notice that cuts it off), and the status page of a Persona that
  learned a skill (its new level, the skill learned and which stats rose are read out; the arrows go through the whole
  page, and Tab jumps its sections). Space or Enter goes on and stops the reading; pressed before the game takes it, it
  is kept until it does. Enemies that fall together are said in one line ("2 Cowardly
  Maya defeated"), and the All-Out Attack prompt is said once with its buttons (Space to attack, Escape to hold back).
- Tartarus: Shadows are points of interest (their own category) and growl from their direction when within 30 m,
  faster and louder as they close in. An alarm sounds when one notices you, and its growl stays urgent while it hunts you.
  Within 4 m you hear whether a Shadow has its back turned or is facing you (each said once as you close in). A blade's ring repeats while it is inside
  your strike's reach (strike with Ctrl and it connects); two quick high ticks instead mean you are also behind it:
  strike for the advantage. A breakable object rings the same way while your strike would break it. Chest items are read as you
  pick them up, and opened chests leave the list.
- The camp menu's submenus (Item, Equip, Skill, System and others): the highlighted entry, its position and its
  description. In Equip's gear lists F (X / Square) reads the highlighted item again with its description. System > Tutorial lets you re-read every tutorial, in tabs (Battle, Dungeon, Daily Life,
  Fusion, System) that Q and E (LB and RB) switch, each named as you reach it; an empty tab says so. Button icons in
  tutorials are read as key names.
- Camp Item and Skill: items with how many you own; when you choose who casts a skill or who gets an item or a
  healing skill, each member is read with their HP and SP (no level), and the new HP once it lands.
- Learning a skill (a skill card from Item, or a Persona's level up): the skill, then the Persona with its stats
  and skills; Q and E switch Persona, Space learns. When the eight slots are full and the game asks which to
  forget, the arrow keys move over the slots (two columns of four), each read with its cost; Space picks it, C goes
  back. Choosing who takes a skill card reads each Persona with its skills.
- Stats: each party member with level, HP, SP and their Persona ("Yukari, level 3, HP 74 of 74, SP 61 of 61, 2 of 3,
  Io, Lovers, level 3"). Ctrl+Down or the right stick down walks the rest line by line (the Persona's stats, each
  skill with its cost, its affinities), Ctrl+Up or the stick up goes back; Space opens their
  page, read as lines to move through: the member, (for you) your social stats, their Persona, its stats, each skill
  with its description, its affinities; Q and E switch member. Once someone in the party knows a Theurgy, Z on a
  member's page opens the Theurgy list: "Makoto, Cadenza, 1 of 2" and its effect; left and right arrows choose, Q and E switch
  character, F reads their personality (what fills their Theurgy gauge), Escape goes back. Controller: LT opens it,
  D-pad left and right choose, LB and RB switch character, X reads the personality, B goes back.
  Q and E switch to Tactics: each member with their tactic ("Yukari: Direct Commands"); Space opens the tactics. System > Rewind: each day you can go back to.
- Config (System > Config): each settings page (Game System, Sound, Graphics, Display, Key Configuration), then
  each setting with its current value and position ("Vibration: on, 1 of 24"); its description ("Turn controller
  vibration on or off") is on the details keys; left and right arrows change the value and the new one
  is read; Space saves the changes and H restores defaults. Key Configuration reads the key bound to each action. The
  BGM rows open a playlist: each track as you move, Space sets it, F plays it (see the known issues about reaching it from the keyboard). System > Dictionary: each term as you move, and its
  explanation when you open it.
- Shops (the police station): the menu, items with price and stats, descriptions, the buy and equip prompts.
  Aohige Pharmacy (Paulownia Mall): each item with its price and how many you own, its description, then "How
  many?" with the total as you change the number (W and S), and the Buy or Quit choice. Mayoido Antiques
  (Paulownia Mall, from July): its menu (Fuse Equipment, Trade Card, Trade Item), each entry with how many you own,
  its stats and what the trade needs ("needs Topaz 3, you have 1"), then its description. Be Blue V is a part-time
  job, read as choices. Money gained or spent anywhere is announced.
- People in the points of interest list carry what the icon over their head says: "talked to" once you have heard them out today, "Social Link" when a link can move forward, "Social Link, reversed", "hangout", "Linked Episode", "request", "study group", "walk". No word after the name means they have something new to say.
- Tartarus: chests are listed by kind ("Treasure Chest", "Rare Treasure Chest", "Gold Treasure Chest", and "Locked Treasure Chest, 2 Twilight Fragments" with what it costs to open).
- Tartarus: Shadows are listed by kind ("Shadow", "Strong Shadow", "Rare Shadow", "The Reaper").
- Tartarus: a missing person waiting on a floor is listed under People ("Missing Person", "Stray Cat") until rescued; walk into them to rescue them (not yet tested in the game).
- Tartarus: the block and floor ("Thebel, floor 3"), breakable objects as points of interest (broken ones leave the list; a crystal that gives a Twilight Fragment is listed as "Twilight Fragment crystal"), stairs ("Head Up") and the Monad door ("Enter Door") under Exits.
- Outside battle, a party member's HP or SP changing is read (Auto-Recover with Y, or RB on a controller, in Tartarus; items, the
  entrance clock). N (Navi) and the other Tartarus helpers speak through messages.
- Tartarus terminals (the Giant Device at the entrance and on floors): each floor you can travel to, the furthest
  one reached marked; the up and down arrows choose, Space travels.
- A Social Link event that grants a Theurgy says so with its effect ("New Theurgy: Cyclone Arrow. Deals severe Wind
  damage to 1 foe, ignoring resistances.").
- Battle Tactics (W): "Give an order?" (Order or Escape), each party member with their current order, and the
  orders (Direct Commands, Act Freely, Full Assault, Conserve SP, Heal/Support). Rush (Tab) lets the party fight on
  its own.
- The camp Calendar: the month as the game lays it out: left and right move a day, up and down a week, each day
  read with its weekday, then its plans; Q and E change month; F on a day with plans reads them. The camp Persona menu shows one Persona at
  a time as its status page, read as lines to move through, top to bottom, in sections that Tab and Shift+Tab (RT and
  LT) jump between: the Persona with its arcana and level ("equipped" on the one you have on; Space on this line equips
  it) and the EXP to its next level; Stats, one per line ("Strength 54" to "Luck 23"); Skills, each with its cost and
  description; Learns later, each skill and at which level; Affinities, one kind per line ("Weak to Light", "Repels
  Fire Wind"). Q and E (LB and RB) switch to the previous / next Persona, wrapping. The same page
  is used in the Velvet Room (fusion results, F on a fusion list, the compendium, registration).
- The camp Quest's Missing Persons tab (E): every case with its number, name once known, deadline and status
  (searching, found, discontinued), and the profile of each person you have identified. Backslash or F (X / Square)
  repeats it.
- Elizabeth's requests (from May 10, and the camp's Quest list): each request's title as you move, then its task;
  F reads the full request and its reward; Space accepts.
- The Velvet Room (the blue door in Paulownia Mall): its menu (Fusion, Summon from the Compendium, Register to the Compendium), the fusion method,
  and in Dyad Fusion each Persona with arcana and level; when choosing the second Persona, each row also says what
  the pair makes ("Apsaras, Priestess, level 2, makes Nekomata, Magician, level 3") or "cannot fuse", and whether
  the result is above your level or already in your stock. On any fusion list Right steps through the details
  one at a time (on the second Persona's list, the result's: "Result, Nekomata, Magician, level 3", then each stat,
  skill and affinity); Left steps back. The result page is the status page above, with the Social Link bonus
  ("Social Link bonus: rank 8, 32793 EXP, up to level 28"); Q and E (LB and RB) page to its materials, each said as
  "Material" rather than "Result". Then each skill you can inherit with its cost and
  description, "chosen" or "not chosen" as Space changes it; when Igor asks "Is this Persona acceptable?" the mod
  adds what it inherits. "Fusing." marks the start of the fusion's animation (Tab skips it). After the fusion, the Social Link bonus's level up is a page too: the new level, each
  stat with what it gained ("Agility 21, up 7"), and new skills marked "new"; when its eight skill slots are full
  you choose one to forget, slot by slot. Fusion Search names each Persona your stock can make and from which two;
  Special Fusion lists each recipe with its materials and how many of them you have. The compendium reads registered
  Personas with their summon price (others by arcana and level) and says which are already in your stock, and
  Registration lists your Personas, then their status page before you confirm; Right walks a Persona's details on
  both lists too.

## Top-down exploration

Persona 3 Reload's camera sits behind the hero and turns, so a sound "ahead" kept changing direction. In the
top-down mode (the Top-down exploration setting, on by default) the field is heard the way Dark Vision presents
Wrath of the Righteous:

- While you walk freely, the camera is fixed facing north and cannot be turned. The game moves you relative to its
  camera, so up (W, or the left stick up) always walks north, right walks east, and so on.
- The game's own sounds (footsteps, voices, doors, Shadows) are heard from where you stand, with north ahead, as if
  from above; so are the mod's sonar, Shadow cues and wall tones. A Shadow's growl comes from where the Shadow is.
- The wall tones are the compass: north, east, south and west of you. As in Dark Vision, they play only while you
  are moving and go quiet when you stop; they reach 3 m, and the tone for the direction behind your walk stays silent
  (walking north you hear north, east and west; walking north-east, north and east).
- The points of interest list gives directions by the compass ("Iwatodai Station, 8 meters, north-east").
- The right stick scans a direction (see On a controller).
- North is the map's up (the map screen and the corner map draw it that way).

Conversations, story scenes, menus and battles are untouched: the camera and the sound go back to the game's own the
moment one starts, and come back to north when you walk freely again. Turn the setting off to play with the
game's own camera.

## Sounds

- Wall tones: four looping tones tell you how close walls are: ahead and behind (centered, different pitches),
  right (right ear) and left (left ear). Louder means closer; silence means open space. Each tone measures how far
  holding that direction would actually take you: a wall you would just slide along stays quiet.
- Sonar: every point of interest within range (10 m unless you change it) repeats its own sound from where it is, starting the moment it comes into range.
  The closer it is, the faster it repeats and the louder it is, so walking toward a sound makes it quicken. The speed
  goes by distance alone, whatever the range is set to: about once a second at 10 m, twice a second at 5 m, three
  times at 3 m, five times at 2 m and eight times at 1 m, so each halving of the distance nearly doubles it. Inside
  2 to 3 m a thing plays a clipped form of its sound, so that it can repeat that fast: one knock, one whistled note,
  one (the higher) bell strike, a puff of air, the start of the pluck, one turn of the trill, the last two chimes. Each kind has a sound of its
  own: exits (ways into another area) a rising breath of air over a low thump, doors two knocks on wood, people a
  two-note whistle (the second note lower, a "yoo-hoo"), shops two strikes of a small bell, objects (things to check or open) a bright
  plucked note, events (a Gatekeeper, a story scene you walk into) a quick trill between two hollow notes, and Twilight Fragments three high chimes of glass, rising, a sparkle. Directions follow the camera: a sound straight ahead is where pushing forward walks, and turning the
  camera turns the whole field around you. A sound played lower is behind you (Lower pitch behind you). To keep
  crowded places readable only the six nearest things sound, at most three of one kind; things on another floor are
  left out unless you can walk there, and so are things past a closed door (the corridor's from the gym hallway), until you go through it. The point selected in the points of interest list always has a place among
  them. Each thing keeps its own rhythm, however many others are near: things of different kinds sound independently, over one another when their moments meet, and two of one kind are always kept a moment apart, so they never start as one sound. A thing that has just come into range sounds at once. The sonar is silent in menus, conversations, story scenes and
  battles. Shadows are not part of it: they keep their own pulse.
- Shadow pulse: a low, rattling snarl from the nearest Shadow within 30 m, whatever its height, faster and louder as it closes in
  (from every 0.8 seconds at 30 m to one unbroken snarl beside you), lower when it is behind you. When a Shadow notices you, a harsh two-note alarm sounds from where it is, and its growl turns higher,
  louder and nearly twice as fast for as long as it hunts you; two soft falling notes mean it has lost you.
- Strike reach: while a Shadow stands inside the reach of your strike (the game's own hit area, counted a little short so the ring never comes at its very edge: about 2.3 m ahead of
  you and 1.2 m to each side, measured to the Shadow's middle), a quick ting of steel repeats fast (about six a second) and stops the moment you leave the reach: press the attack now and it connects. It is
  about you, not the Shadow, so it plays in the middle of your head, the same in both ears, not from the Shadow's direction. When you
  are behind the Shadow the ring is two quick high ticks instead, the second higher: the strike also brings the
  advantage. No ring means a swing would miss. The same ring plays for a breakable object while your strike would
  break it (the game's own hit area again), unless a Shadow is in reach: then the ring is the Shadow's. It is part of
  Shadow alerts, so it is silent while that setting is off.
- Sounds stay in place: a sound that has a place in the world (the sonar's, the Shadow cues) stays there for as long as it plays, so it shifts in your ears as you walk or turn the camera during it.

The sonar and the Shadow cues are positioned with Steam Audio (Valve, Apache 2.0; phonon.dll beside
the mod; its license and third-party notices are in the P3RAccess\licenses folder). Without phonon.dll they pan left and
right as before. Headphones give the best sense of ahead and behind.

## Known limits

The most important: the 3D story scenes have no audio description, only the animated movies do. Signing the contract with Done on a controller has not been tested. A game update can break the mod. The full list, including what has not been tried yet, is in [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

## Reporting bugs

Please open an issue at https://github.com/ChipsAhoiMcCoy/persona-3-access/issues

These help most:

- What you did, what you heard, and what you expected.
- Your screen reader, and whether you use a keyboard or a controller.
- The log, from `%LOCALAPPDATA%\P3RAccess\p3access.log`. After you restart the game, the previous session is `p3access.last-play.log` in the same folder. The log holds what the mod spoke, which includes story text, and may show your Windows user name in a path, so look it over before sharing it.

## Credits

Persona 3 Access builds on Grim Dark by Austin Hicks and Dark Vision by Bradley Renshaw. It uses Prism, Steam Audio, Microsoft Detours and miniaudio. The movie narrator is the Kokoro voice model's af_heart voice. Thanks too to the Persona 3 Reload modding community. Full credits are in [CREDITS.md](CREDITS.md) and licenses are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License

The mod's own code is under the zlib license: see [LICENSE](LICENSE). Parts come from other projects under their own licenses, listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The license covers the mod only, not the game or anything of the game's.
