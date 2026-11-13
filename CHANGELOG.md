# Changelog

## Unreleased

### Top-down exploration (new; to be tried)
- While you walk freely, the camera stays facing north and cannot be turned: up always walks north. The game's own sounds and the mod's are heard from where you stand with north ahead, as if from above, so a Shadow's growl comes from the Shadow. Conversations, story scenes, menus and battles keep the game's camera and sound. The Top-down exploration setting (Exploring) switches it off.
- Up walks north everywhere. Where the game shows a place from a fixed scene camera of its own (Fountain Square, the station, many rooms), up walked toward that camera's view instead, so sounds slid past sideways while you walked straight.
- North is the corner map's up in every place: where the game draws a map turned (Port Island Station, your room, the Command Room and others), north turns with it.
- The right stick scans a direction: push it toward one to hear the nearest point of interest that way, its distance and how far the wall is, and to hear that point's sound from where it is.
- Directions in the points of interest list are given by the compass ("north-east") in this mode.
- The wall tones in this mode play only while you move, reach 3 m and leave out the direction behind your walk: fixed to the compass, they had found a wall in three or four directions all the time.
- In this mode the wall tones went quiet when you walked straight into a wall, just as it was nearest, because the hero had stopped moving. They now play while you push a direction, whether or not the hero gets anywhere.
- New sound placement, from Dark Vision: left and right by level and by the moment each ear hears a sound, sounds behind darker. The Sound placement setting (Sound) chooses it, Steam Audio's 3D (as before) or a plain pan. It replaces the 3D sound setting.

### Sonar
- A thing is heard from its part nearest you, not from its middle, as in Dark Vision: a long counter or a wide exit sounds from beside you and quickens as you near its edge, so it is heard sooner. Standing inside a thing's area (Paulownia Mall's exits are strips you walk through), its sound comes from its middle at the quickest rhythm. People and Shadows still sound from where they stand.
- Sounds always play in full. Up close, where a thing repeats fastest, a shortened form of its sound played instead, so a door's knock and the exit's and event's sounds were cut off. Now the whole sound plays every time at its own pitch, as in Grim Dark, and a long one starts again over its own tail when its rhythm is faster than it is long.
- Grim Dark's sonar sounds: exits use its dungeon entrance sound, objects its interactable sound, events its ruined shrine, Twilight Fragments its loot, and a near Shadow its enemy ping. Doors, people and shops keep their own sounds, as Grim Dark has none like them.
- Each thing keeps its own rhythm again, as in Grim Dark: things of different kinds sound independently, and only two of one kind are kept a moment apart. In 0.1.0-alpha.3 only one sound played at a time, so in a crowd every thing waited its turn and the rhythms fell into a slow queue.
- The Sonar rhythm setting is gone with the sweep rhythm; pulse is the only one.
- Every thing repeats a quarter more slowly than before at each distance: almost seven times a second at 1 m, about once a second at 10 m.

### Sound
- Stairs no longer sound like a wall. The wall tones read the bottom of a staircase as a wall right there, in either mode: the stairs up out of a Tartarus floor, and low steps such as the ones your party stands on near the top of Tartarus. They now follow the steps up.
- If you muted the game in Windows' volume mixer, starting it un-muted it a few seconds in. Your mute now stays; only a mute the mod itself left behind is lifted.
- The mod's sounds (wall tones, sonar, cues, audio description) came back only after a restart when the output device went away, or when there was none at launch, such as a Bluetooth headset that connected late. They now come back on their own within a few seconds of the device returning.
- When a movie was skipped back or forward during a description, the description stopped with a click. It now fades out under the next one.
- In stereo sound placement, something close beside you now sounds from that side: a door 1 m to your right was placed less than halfway right, less than one 10 m away.
- Things behind you are easier to tell from things ahead. In the stereo placement the darkening behind now reaches the sonar's sounds; before, most of them sounded almost the same ahead and behind. In top-down mode, sounds to the south now play lower too, with "Lower pitch behind you" on, as in the camera mode.

### Points of interest
- On a controller, holding both triggers selected the nearest point every time, so choosing a point, letting go, then holding the triggers and pressing A walked to the nearest point instead of the one you chose. Opening the layer now says your selection and keeps it; with nothing selected it still picks the nearest.
- Moving to the Events category said a story event's name twice over ("Events: Event: Jin"). It is now "Events: Jin".
- Empty spots where the game can place a character were listed as "Person" when nobody stood there (three on the dorm's third floor, a step from the hallway), and walking to one stalled. They are no longer listed.
- People in your own room were left out of the list when the desks split the floor, as in Classroom 2-F: walking from the back door to Junpei's desk, the way around led out through one door and back in through the other, so he seemed to be behind a closed door. Anyone you could see over the furniture is listed now, and walking there finds its own way between the desks: from the back door to every desk, both bookshelves and the front door's students. Only where your body truly does not fit does it say "No path, though it is in this room". A student standing by the front door no longer hides the rest of the room.
- A missing person in Tartarus is listed with their name from the police notice once the notice shows it: "Missing Person, Ayako Yoshimoto".

### Battle
- The countdown in the battle against the Priestess on the monorail is spoken: "Time left: 29 minutes 59 seconds" when it starts, the minutes left with each of your turns ("29 minutes left"), every 5 minutes, then at 1 minute, 30 and 10 seconds, and the new time whenever the story cuts it short.

### Shops
- When a script asks you to pay with your money shown (the late-night TV shopping's "Buy / Don't buy"), the price and what you have are said before the answers: "Price 2980 yen, you have 4800 yen".

### Character descriptions
- The area key (Semicolon, or both triggers and B) now describes the character talking to you while their portrait is up beside the line: how they look, the outfit the portrait shows (summer or winter uniform, casual clothes, swimsuit, yukata, combat gear and the rest) and how they stand. It follows their outfit as it changes through the year. All 155 outfits of the 53 characters with a portrait are described. Outside a conversation the key describes the place, as before.

### Fusion
- A skill change offered after a fusion (Igor: "A skill seems to be on the verge of changing...") is followed by what each skill would become: "Bufu would become Spirit Drain. Agi would become Dodge Ice". Tried in the game, with the new Persona levelling up first and without.
- Choosing skills to inherit now says how many you may take ("Choose skills to inherit, up to 2.") and counts as you choose ("chosen, 1 of 2 picks"). Taking the last one brings Igor's question at once, as before.

### The end of the game
- A save made after the credits was read like any other ("March 5, level 96"). The list now says what its row shows: "Slot 9: Makoto Yuki, Clear data, start a new game, play time 46 hours 16 minutes".
- The credits: the review key now reads the names rolling past at that moment ("Credits. Chief Designer, Yuichiro Takada, Art Director, Tomohiro Kumagai, ..."). The roll is not read by itself.
- The last choice of the ending opened with no answer picked and Enter did nothing until Left or Right was pressed. The first answer is now picked as it opens, and Enter takes it.
- On that same choice, Left or Right changed the game's answer while the mod kept reading the first one, so Enter could take an answer you never heard. The mod now follows the game's answer.

### Lists
- A "LIST" choice longer than its window (the Velvet Room's Talk, 16 topics) listed only the rows on screen as it opened: the topics further down could not be reached. The whole list is read now.

### Town map
- F (town info) now reads a destination's shops and places from the game's own list instead of the text on screen, in the order the panel shows them, with the ones closed now marked "unavailable". It is said as soon as the panel comes up or the cursor moves, without waiting for the panel's animation.

### Shopping
- The TV shopping show (Sunday afternoons, the TV in your room) has now been played through with the mod: the show, the Buy / Don't buy choice and the purchase are all read.

### Mod settings
- Letter keys no longer jump to a setting by name in the mod settings menu; they do nothing there. The arrows, Home and End, Tab and the rest are unchanged.

### Calendar
- F on a day (the part-time jobs) read only the highlighted job's description, sometimes with the day's "Nothing in particular" still fading behind it. Each job is now read by name with its place in the list and "unavailable" when that day does not take it: "Chagall Café (unavailable), 1 of 2. Serve coffee that boosts a person's charm! Time: Night ...".
- When the story moved to another day with only the time-of-day screen (the epilogue's jump to March), just "After School" was said. The new date is now read with it.

### Less said twice, fewer keys read out
- Key names are no longer part of spoken lines; F1 reads them, as everywhere else. A turn's line ends "Theurgy ready" (it was "Theurgy ready, T"), a 1 More says "Shift available" (it was "B shifts the turn to another member"), a shop's quantity opens with "How many? 1" (it was "How many? W and S change, Space confirms. 1"), and the Stats up screen no longer ends with "Space continues."
- Beside a Shadow that kept turning, "Shadow close, facing you" and "Shadow close, back turned" were said every time it turned: over a hundred times in a minute next to one that circled. Each is now said once as you close in on a Shadow; the ring that tells you a strike will connect, and whether from behind, still follows its facing all the time.
- A Shadow that chased you out of earshot and came back was treated as if it had not noticed you. It is still growled as hunting.
- After a Persona with eight skills learns one at a battle's end and you choose the skill to forget, the victory screen no longer says "Victory" again, and the Persona's "Level up!" line is read once (it came twice).
- "Theurgy obtained" no longer cuts off the line being read, and the town map's information about a place no longer cuts off the place's name.
- The victory page, a saved-game line, an opened mail and a few other lines of a menu's own can no longer be read twice in a row when nothing was pressed in between.
- A thought or notice of two pages that followed another one (Club Escapade's closed door, the dorm fridge's notice) was read whole while its first page still showed, and its second page again, as a new line, once it came up. Each page is now read when the game shows it.
- Walking to the Velvet Room from Paulownia Mall stopped in the back alley, short of the door, and needed a second press. A walk now goes on to its target after crossing into another part of the map on the way.
- An item added to a "Got ..." notice that is already up is said alone ("Got 1 Up"), not with the whole list again.
- The point of interest list keys (the bracket keys) say "Not in the field" in a menu, a conversation or a battle, where the list is not about what is on screen.

### Battle
- Shuffle Time's Persona Overstock list (a Persona card with a full stock): the game's cursor is now put on the row you focus, also after Home, End or a jump, instead of being stepped there one press at a time.
- When the game greys out commands on the battle wheel, the turn's line now says so ("Attack, Items, Guard unavailable"). It happens in a tutorial's battle and on the very last turn of the final battle, where only the Persona's skills are left and a press of Space did nothing without a word.
- Analyze now gives everything its screen shows. Besides the name, level, health and affinities, the detail buffer (Ctrl+Up and Ctrl+Down, or the right stick) reads the enemy's maximum HP and SP, its arcana, its attack type, its skills and what it drops, each only once the game itself shows it (a Shadow not yet analyzed keeps its skills unknown).
- A battle now opens with who has the advantage, as the splash on screen shows it: "S.E.E.S. advantage" when you struck first, "Shadow advantage" when the Shadow caught you. Nothing is said for an even start.
- When a Persona with eight skills levelled up after a battle and had to forget one to learn the new skill, the list of skills was not presented: only "This Persona cannot learn any more skills" was read, and Enter forgot whichever skill the game's cursor was on. The list is now a menu, as it already was with a skill card: the new skill first, then the eight slots with their costs; Enter asks "Forget ...?", and the level-up page follows.
- A physical skill's cost in the battle skill list was read as a percentage ("Akasha Arts, 140% HP") when the number is the HP it takes from that member. It now reads "Akasha Arts, 140 HP". The Persona pages keep the percentage ("22% HP"), which is what they show.

### Points of interest: what the list was missing
- The icon the game floats over a person is now read after their name in the list: "Akihiko Sanada, talked to" once you have heard what they have to say today, "Social Link" when a link can move forward, "Social Link, reversed", "hangout", "Linked Episode", "request", "study group" and "walk". A person with nothing after the name has something new to say.
- The two Twilight Fragments in the underground base of the August full moon were listed as "Something Shining", as plain objects. They are now "Twilight Fragment", with the fragment's sound on the sonar, like the ones in town.
- A missing person waiting on a Tartarus floor was not in the list at all. They are now listed under People by the game's own name for them ("Missing Person", "Stray Cat", "Old Man in Pain"), sound on the sonar as a person, and leave the list once rescued. Not yet tested in the game.
- Tartarus' Monad door ("Enter Door") was listed under Objects with the object sound. It is a way onward: it is now under Exits with the exit sound, and so are the doors inside the passage ("Exit Door", "Proceed Onward", "Go Back").
- Every Tartarus chest read "Treasure Chest". The list now tells them apart: "Treasure Chest", "Rare Treasure Chest" (the silver case under purple sparks), "Gold Treasure Chest" (the gold case; the one opened held a Diamond) and "Locked Treasure Chest, 3 Twilight Fragments", with what it costs to open. The prompt beside a chest still reads as the game shows it, "Check: Treasure Chest".
- Every Shadow on a floor read "Shadow". The game's stronger kind is now "Strong Shadow"; the rare kind that runs from you is "Rare Shadow" and the Reaper is "The Reaper" (those two not yet met in the game).

### Points of interest: only the part of the map you are in
- Some places are two areas in one map with a closed door between them, like the school's Gym Hallway and the Corridor outside it. Everything past the door was listed and sounded on the sonar, and the auto-walk to it ran into the door and said "Stuck, stopping". Things past a closed door are now left out of the points of interest list and the sonar until you go through the door's exit; then that side's things are listed and the other side's are left out.
- The exit that takes you through the door stays in the list.
- A point you selected before going through a door is no longer selected once the door is between you and it.
- A Twilight Fragment's prompt is said with its verb, "Check: Something Shining", like any other thing's. It is said again when you walk away from the fragment and come back; it used to stay silent until some other prompt had shown.
- Fixed: after a prompt went away the mod looked its thing up in a fresh list of points of interest every frame, and never stopped once that thing had left the list. It now asks the thing itself.

### Objectives
- Fixed: with two objectives on screen at once (a story objective and a Tartarus floor's "Explore the floor"), both were said again and again without end. Each line of the objective panel is now said once, when it appears.
- A fourth line of the panel ("Find a valuable treasure chest") is read too; it was below where the mod looked.

### Auto-walk: round people in the way
- At the top of Tartarus the walk to "Event: Takaya" went straight at it, into the pillar in the middle of the landing, and said "Stuck, stopping". A story trigger that floats well above its floor is now walked to by the floor under it: the walk goes round the pillar and up the ramp.
- The auto-walk's path knows the walls but not who is standing on the floor. With a student walking toward you down a hallway, it pushed into them and said "Stuck, stopping". It now sees someone in the next step of its path and steps to one side of them, then goes on.
- When it is blocked by anything else its path does not know of, it tries a step to either side before giving up. It says "Stuck, stopping" after three such steps that brought it no nearer.
- Fixed: arriving at a Social Link partner sitting at a desk (Kenji in Classroom 2-F) turned you away from them, so the Talk prompt went away and they could not be talked to. You are now turned to face them.

### Mail
- Fixed: the phone's mail list read "Mail 1" in place of the sender and subject about every other time it was opened, and a mail opened from such a list read only the sender's name, without its subject or text.
- Fixed: opening a mail read nothing at all. The sender, the subject and the mail's text are read when it opens.
- With no mail, the list says "No messages"; it also read the keys to close it, which F1 reads now.

### Town map
- Each destination says who is there now, after its name and position: the Social Links you can spend time with ("Social Links here: Kenji Tomochika (Magician), rank 1, ready to rank up") and the other characters the map marks. A link you have not started yet is listed too, as the game marks it, under the game's own name for it ("Bookstore (Hierophant)"). The game shows them as card icons; nothing read them before.

### Shops
- A row you cannot buy says why, where it said "not now": "out of stock", "carrying the maximum" (99 of it) or "not enough yen" ("Nastea, 110 yen, 3 owned, out of stock").
- A vending machine holds a few of each drink and refills once a week. Its rows say how many are left ("Fountain Dew, 110 yen, 0 owned, 2 left"); the game shows only a sold-out mark.

### Sonar: nearness is heard
- A thing repeats much faster as you close in: about once a second at 10 m, twice at 5 m, three times at 3 m, five times at 2 m and eight times a second at 1 m. It was three times a second at best and once a second at the range's edge.
- The speed goes by distance alone. It used to stretch with the Sonar range setting, so the same door at the same distance ticked slower with a longer range.
- The sounds are shorter (0.11 to 0.16 seconds, from 0.16 to 0.28), so more of them fit in a second. They still never play over one another.
- Inside 2 to 3 m a thing plays a clipped form of its sound so that it can repeat that fast: one knock, one whistled note, the higher bell strike, a puff of air, the start of the pluck.
- With many things around, the near ones keep most of their speed and the far ones wait longer. The turns used to be shared so evenly that a thing beside you in a full room repeated less than once a second.

### Twilight Fragments
- The glowing blue fragments lying around town ("Something Shining" on screen) are listed among the objects as "Twilight Fragment" instead of under the game's name. A fragment leaves the list once it is picked up.
- In Tartarus the objects list "Twilight Fragment crystal": the breakable object that always gives a fragment when struck. It was missing from the points of interest and from the sonar altogether, and it had no strike-reach ring. It has all three now.
- Both have a sonar sound of their own, so a fragment stands out from the other objects by ear: three high chimes of glass, rising, a sparkle. There is a "Sonar: Twilight Fragments" setting and a "Sonar: Twilight Fragment" row in the Sound test.

### Events: Gatekeepers and scenes you walk into
- Points of interest have a new category, Events, between Objects and Shadows: spots that start a fight or a scene when you walk into them, with nothing to press. On a Tartarus Gatekeeper's floor it lists "Gatekeeper", where the fight starts, until the Gatekeeper is beaten.
- Events have a sonar sound of their own, a quick trill between two hollow notes. They used to share the objects' plucked note. There is a "Sonar: events" setting and a "Sonar: event" row in the Sound test.
- Fixed: the sonar could tick for an unnamed event spot that the points of interest list did not have. On a Tartarus floor it sounded like an object right beside you that could not be found in the list.
- An unnamed event spot that is already finished is no longer listed.

### Less to listen to
- A battle turn opens with a shorter line: "Junpei, HP 99, SP 17. Cowardly Maya, 35%, Down, weak to Slash, Fire". The HP and SP maxima, the word "Target" and the target's place are gone from it; moving the target still says its place.
- Affinities drop the word "Attack": "weak to Fire, Ice" instead of "weak to Fire Attack, Ice Attack". "Downed" is now "Down".
- The battle skill list reads name and cost only ("Agi, 3 SP, 2 of 3"). What the skill does is one flick away: Ctrl+Down or the right stick down reads it, then the acting member's details.
- The Stats list reads each member with their Persona's name only. Ctrl+Down or the right stick down walks the Persona's stats, skills with costs and affinities line by line. This is the same details list battles have, now outside battle too.
- More lists keep their descriptions for the same details keys instead of reading them after every row: the camp menu's entries, items in the camp menu and in battle, the camp menu's skills, Equip's gear, the skills on a Persona's status page (one just learned still says what it does), the fusion skill-inheritance list and the game's Config. Backslash reads a row with its description in one go, as before. Shops, requests, the calendar, Shuffle Time's cards and event choices still read theirs at once.
- F1 mentions the details keys wherever the focused row has details.
- Choosing who gets an item, casts or receives a skill, or changes gear no longer reads each member's level.
- A skill or item that hits a whole side asks one question and says the buttons: "Use Mazio Gem on all enemies? A to confirm, B to cancel." It no longer says "Choose target", every name on that side and "1 of 1"; each hit says who it landed on, as before. F1 there reads the same two buttons.
- The mod settings are in five categories instead of one list of 27: Exploring, Sonar, Sound, Dialogue and movies, Controls and help. Tab and Shift+Tab, Page Down and Page Up, or RB and LB change category.
- Mod settings: every setting says what it does, read with the row after its value and position. No extra key is needed.
- Mod settings: a new Sound test category. Space or A on a row plays one of the mod's sounds, and the row says what the sound means: the sonar's five sounds, a sound from behind you, the Shadow's growl, alarm and lost-you sound, the strike-reach rings, the four wall tones and the controller layer's tick.
- Mod settings: pressing on at the first or last row says the row again without the cursor sound, as the game's menus do.

### Removed
- The beacon (the chirp toward the selected point of interest), its setting and its key, Shift+Semicolon. The sonar already marks the selected point, and the auto-walk takes you there; the beacon was one more sound on top of them.
- The short tick that marked the selected point of interest among the sonar's sounds. The selected point still always has a place among them, and still plays its sound once when you land on it in the list.
- The nearby chimes and their setting. The sonar's sounds are what tell you something is near; with the sonar off, nothing plays in their place.
- "Beeps and chimes volume" is now "Shadow sounds volume": it sets the Shadow cues and the controller layer's tick, which is all that is left on it. Your saved value is kept.

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
