# Known issues

Persona 3 Access is an alpha. This list says what is missing and what has not been tested. If you reach one of these, your report is especially useful: see "Reporting bugs" in the README.

## Not covered

- The 3D story scenes have no audio description. Only the animated movies do. Dialogue in the 3D scenes is read, but what happens on screen is not described.
- The game's text is read in English only, and the mod's own speech is English only.
- On a controller, typing a name in the contract still needs a keyboard.

## Written but never tried in the running game

These reached the code but have not been seen working in the game, because no test has reached them yet:

- Signing the contract with Done on a controller. The keyboard path has been tested.
- Cooking, which runs through invitation events.
- The TV shopping show.
- Linked Episodes.
- Exam results.
- The final battle, the ending, and the end credits' list of names.

## Tested less than the rest

- Episode Aigis has not been played through with the mod. Its movies and area descriptions are in place, and its Theurgy list has been checked, but the episode as a whole is untested.
- A late game playthrough has not been completed. Each menu was tested on saves from various points, not by finishing the game with the mod. Monad and the Abyss of Time were visited to check area descriptions, not played through.
- In Analyze, the stop at the first and last enemy has not been tried in the game. On the command wheel it has.
- The single "All enemies" or "All allies" choice for a skill or item that hits a whole side has not been tried in the game.
- The victory screen's new pacing was tried in one battle with a member's level-up. The hero's own level-up page and a battle with several level-ups at once have not been tried with it.
- The compendium's Persona status page (Summon from the Compendium, then a Persona) was not tried in the game after its fix. Registration's page, which had the same fault, was.
- The auto-walk's search for the way in to a walled-in target was tried on one Tartarus teleporter, from one side.
- The Missing Persons board in the field. The camp's Missing Persons list has been tested.
- A missing person waiting on a Tartarus floor, the "Rare Shadow" and "The Reaper" names in the points of interest list, and the sound for a Shadow that loses track of you have not been met in a test.
- Shuffle Time's reading of a card whose text is the same as the card before it was changed and has not come up in a test since.
- The audio description text was checked by automated timing and transcript checks, not by a person watching every movie.
- This Release build. Development used a slightly different build type, so the packaged zip is its own test.
- The menu animation wait in a simple shop opened by the story.
- Screen readers other than the ones used day to day. Please say which one you use when you report a problem.

## Known bugs

- The BGM playlist in Config: the game opens it with D, but the mod's menu keeps W, A, S and D from the game. It may not be reachable from the keyboard. This comes from reading the code and has not been tried in the game.

## Behaviours to expect

- While the mod settings are open, the game keeps running. In Tartarus, open them from the camp menu's Config to be safe.
- How many skills a fusion lets you inherit is not announced in advance. The mod says what was inherited at Igor's confirmation.
- The mod does not say which skill a Persona's "skill change" after a level up will become.
- A voiced line is not read by default, even if its voice cannot be heard, such as when voice volume is at zero. Set "Voiced dialogue" to "read aloud" in the mod settings in that case.

## Game updates

The mod finds parts of the game's code by their exact shape in this version of the game. A game update can break some or all of it. If the mod says it could not find the game engine, or a screen goes silent after an update, please report it, and include the log.

On any version it was not made for, such as the demo, the mod says so when it loads and turns its menus off. Messages and the field still work.

The mod has only been tested on the Steam version on Windows. Other stores and Steam Deck or Linux have not been tried.
