# Known issues

Persona 3 Access is an alpha. This is what is missing and what has not been tested. If you reach one of these, your report is especially useful (see "Reporting bugs" in the README).

## Not covered

- The 3D story scenes have no audio description, only the animated movies. Their dialogue is read, but what happens on screen is not described.
- English only, for both the game's text and the mod's own speech.
- On a controller, typing a name in the contract still needs a keyboard.
- The top-down right-stick scan has no keyboard key yet.

## Never tried in the running game

- Signing the contract with Done on a controller (the keyboard path is tested).
- Cooking, which runs through invitation events.

## Tested less than the rest

- Top-down mode: tried in Tartarus and the dorm lounge by the developer, not yet played.
- Episode Aigis has not been played through. Its movies, area descriptions and Theurgy list have been checked.
- The end of the game (Jin, Takaya, Nyx, the epilogue, credits, clear save, New Game Plus) was played once with test shortcuts, not at a player's pace.
- No full late-game playthrough: menus were tested on saves from various points. Monad and the Abyss of Time were visited only to check area descriptions.
- In Analyze, stopping at the first and last enemy (tested on the command wheel).
- The single "all enemies" or "all allies" prompt for a skill or item that hits a whole side.
- The victory screen's pacing was tried in one battle with one member's level-up, not with the hero's level-up page or several at once.
- The compendium's Persona status page after its fix (Registration's, which had the same fault, was tested).
- Auto-walk finding its way into a walled-in target: tried on one Tartarus teleporter, from one side.
- The Missing Persons board in the field (the camp's list is tested).
- The "Rare Shadow" and "The Reaper" names, and the sound of a Shadow losing you.
- Shuffle Time reading a card whose text matches the card before it, since that was changed.
- Audio description text was checked by automated timing and transcript checks, not by a person watching every movie.
- The Release build: development used a slightly different build type, so the packaged zip is its own test.
- The menu animation wait in a simple shop opened by the story.
- Screen readers other than the ones used day to day. Please say which one you use when you report.

## Known bugs

- The BGM playlist in Config: the game opens it with D, but the mod's menus keep W, A, S and D from the game, so it may not be reachable from the keyboard. Found by reading the code; not tried in the game.

## Behaviour to expect

- The game keeps running while the mod settings are open. In Tartarus, open them from the camp menu's Config to be safe.
- Voiced lines are not read by default, even if the voice is silent (voice volume at zero). Set "Voiced dialogue" to "read aloud" then.

## Game updates

The mod finds parts of the game's code by their exact shape in this version, so a game update can break some or all of it. If the mod says it could not find the game engine, or a screen goes silent after an update, please report it with the log.

On any other version, such as the demo, the mod says so when it loads and turns its menus off. Messages and the field still work.

Only the Steam version on Windows has been tested, not other stores, Steam Deck or Linux.
