#pragma once
// Every string the mod itself speaks lives here (game text passes through verbatim). English only.
namespace p3a::strings {
// Key hints come in two wordings: kX() names the keyboard keys, or the controller's buttons (Xbox names) while the
// player is on a controller (input.cpp decides: whichever the game was last driven with).
bool pad_hints();
struct Hint {
  const char* keyboard;
  const char* pad;
  const char* operator()() const { return pad_hints() ? pad : keyboard; }
};
// Title flow
inline constexpr const char* kTitlePressAny = "Persona 3 Reload. Press any button.";
inline constexpr const char* kTitlePressAnyAstrea = "Persona 3 Reload, Episode Aigis. Press any button.";
inline constexpr const char* kTitleOpening = "Opening movie. Press any button to skip.";
// The two boot notices are images: what they say, in the mod's own words.
inline constexpr const char* kTitleCaution =
    "Caution. The game shows violence, bullying and suicide, so please take care and take breaks when you need "
    "them. It is fiction: any likeness to real people, groups, events or terms is coincidence.";
inline constexpr const char* kTitlePhotosensitive =
    "Photosensitivity warning. The game has flashing screens and bright effects. Rarely, strong or flashing light "
    "or a TV screen can cause brief muscle cramps, loss of consciousness or other symptoms in some people. If that "
    "has ever happened to you, ask your doctor before playing. If it happens while you play, stop right away and "
    "get medical help.";
// The boot logos, one name each (the user's call, 2026-10-29: the longer descriptions were too much).
inline constexpr const char* kTitleLogos[] = {"Atlus", "P-Studio", "Unreal Engine"};
inline constexpr const char* kTitleMenu = "Title menu";
inline constexpr const char* kTitleItems[] = {"New Game", "Load Game", "Config", "Change Episode"};
inline constexpr const char* kSelectDifficulty = "Select game difficulty";
inline constexpr const char* kDifficulties[] = {"Peaceful", "Easy", "Normal", "Hard", "Merciless"};
inline constexpr const char* kContract = "Contract.";
inline constexpr const char* kFirstName = "First name";
inline constexpr const char* kLastName = "Last name";
inline constexpr Hint kTypeThenEnter{"Type it, then press Enter.", "Type it on the keyboard, then press Enter."};
inline constexpr const char* kBlank = "blank";
inline constexpr const char* kNameDone = "Done";
inline constexpr const char* kNamesMissing = "Both names are needed first.";
inline constexpr Hint kPressAnyKey{"Any key continues.", "Any button continues."};
inline constexpr Hint kNameEntryOptions{"Up and down choose a name or Done. Enter retypes a name, or signs on Done.", "Up and down choose a name or Done. A retypes a name, or signs on Done."};
inline constexpr const char* kSaveGame = "Save game";
inline constexpr const char* kLoadGame = "Load game";
inline constexpr const char* kCarryOverSave = "Choose the save data to carry over";
inline constexpr const char* kSlot = "Slot";
inline constexpr const char* kSaved = "Saved";
inline constexpr const char* kEmpty = "empty";
inline constexpr const char* kLevel = "level";
inline constexpr const char* kPlayTime = "play time";
inline constexpr const char* kHoursAnd = "hours";
inline constexpr const char* kMinutes = "minutes";
inline constexpr const char* kHour = "hour";
inline constexpr const char* kMinute = "minute";
inline constexpr const char* kCamp = "Menu";
inline constexpr const char* kCampItems[] = {"Skill", "Item", "Equip", "Persona", "Stats", "Quest", "Social Link", "Calendar", "System"};
inline constexpr const char* kCampHelp[] = {"Use a Skill", "View or use items", "View or change equipment", "View or change Personas",
                                            "View stats or organize the party", "View requests", "View Social Links", "View the calendar",
                                            "View settings"};
// Calendar: the part-time jobs by ArbeitInfo HelpMSGLabel (their names are images; the places' names as the game's
// dialogue writes them), and when each takes work (EArbeitOrderableTime: 1 afternoon, 2 evening, 3 both).
inline constexpr const char* kJobs[] = {"Chagall Caf\xC3\xA9", "Be Blue V", "Screen Shot"};
inline constexpr const char* kJobTimes[] = {"", "daytime", "evening", "daytime or evening"};
inline constexpr const char* kJob = "Part-time job";
inline constexpr const char* kToday = "today";  // the calendar's current day
// The day change's moon (ECldMoonAge 0..29: 0 new, 14 full, as the game's full-moon nights).
inline constexpr const char* kMoonPhases[] = {"New moon", "Waxing crescent moon", "First quarter moon", "Waxing gibbous moon", "Full moon",
                                              "Waning gibbous moon", "Last quarter moon", "Waning crescent moon"};
inline constexpr const char* kGot = "Got";
inline constexpr const char* kTimes = "times";   // a pickup's count: "Got Bead times 3"
inline constexpr const char* kItemNumber = "item";   // an item with no known name: "item 16387"
inline constexpr const char* kSpent = "Spent";
inline constexpr Hint kGetItemClose{"Space closes it.", "A closes it."};
inline constexpr Hint kMoviePaused{"Movie paused. Enter resumes, Tab skips.", "Movie paused. Menu button resumes or skips."};
inline constexpr const char* kMovieResumed = "Resumed";
inline constexpr const char* kCredits = "Credits";
inline constexpr Hint kMovieKeys{"Enter pauses and resumes, Tab skips.", "Menu button pauses or skips."};
inline constexpr Hint kOpeningKeys{"Any key skips the movie.", "Any button skips the movie."};
inline constexpr Hint kMoviePlaying{"Movie playing. Enter pauses, Tab skips.", "Movie playing. Menu button pauses or skips."};
inline constexpr const char* kTownMap = "Town map";
inline constexpr const char* kTownMapPlaces[] = {"Gekkoukan High School", "Student Dormitory", "Paulownia Mall", "Port Island Station",
                                                 "Iwatodai Station Strip Mall", "Naganaki Shrine"};
inline constexpr Hint kFastTravelKeys{"Up and down arrows choose, Space goes, Escape closes.", "Up and down choose, A goes, B closes."};
inline constexpr const char* kMail = "Mail";
inline constexpr const char* kMailNew = "new";
inline constexpr Hint kMailEmpty{"No messages. Escape closes.", "No messages. B closes."};
inline constexpr Hint kMailKeys{"Up and down arrows choose, Space reads, Escape closes.", "Up and down choose, A reads, B closes."};
inline constexpr const char* kTownMapWhere = "Where should I go?";   // the game's own entrance-list title, matched
inline constexpr const char* kAreaMap = "Area map";
inline constexpr Hint kAreaMapKeys{"Tab zooms, C closes.", "Y zooms, B closes."};
inline constexpr Hint kTownMapHelp{"The arrow keys go through the list, the stick moves on the map, Space travels, F town info, Escape closes.",
                                   "The D-pad goes through the list, the stick moves on the map, A travels, X town info, B closes."};
inline constexpr const char* kSocialStats[] = {"Academics", "Charm", "Courage"};
inline constexpr const char* kRank = "rank";
inline constexpr const char* kRankUp = "up to rank";
inline constexpr const char* kStatUp = "up";
inline constexpr const char* kStatsUp = "Stats up.";
inline constexpr Hint kSpaceContinues{"Space continues.", "A continues."};
inline constexpr Hint kGameOverKeys{"Space skips to the choices.", "A skips to the choices."};
inline constexpr const char* kGameOver = "Game Over";   // the game's own words, when its message file is not loaded
inline constexpr Hint kResultKeys{"The up and down arrows go through the results, Space continues.", "The D-pad goes through the results, A continues."};
// Battle
inline constexpr const char* kBattle = "Battle";
inline constexpr const char* kHP = "HP";
inline constexpr const char* kSP = "SP";
inline constexpr const char* kOf = "of";
inline constexpr const char* kTarget = "Target";
inline constexpr const char* kSkills = "Skills";
inline constexpr const char* kItems = "Items";
inline constexpr const char* kChooseTarget = "Choose target";
// A skill or item that takes a whole side at once: the one thing in the target list.
inline constexpr const char* kAllEnemies = "All enemies";
inline constexpr const char* kAllAllies = "All allies";
inline constexpr const char* kAllUnits = "Everyone";
// EBtlDataAnalyze 0..6: normal, null, repel, drain, weak, resist, unknown
inline constexpr const char* kAttrNames[] = {"Slash", "Strike", "Pierce", "Fire", "Ice", "Elec", "Wind", "Light", "Dark"};
inline constexpr const char* kAffinity[] = {"", "nulls", "repels", "drains", "weak to", "resists", ""};
inline constexpr const char* kDamage = "damage";
inline constexpr const char* kHealed = "healed";
inline constexpr const char* kDefeated = "defeated";
inline constexpr const char* kKnockedOut = "knocked out";
inline constexpr const char* kVictory = "Victory";
inline constexpr const char* kExp = "EXP";
inline constexpr const char* kYen = "yen";
inline constexpr const char* kLevelUp = "Level up!";
inline constexpr const char* kResultPersona = "Persona";   // the hero's level-up page: "Persona Nekomata, level 7"
// EBtlDataBadStatus 0..13
inline constexpr const char* kBadStatus[] = {"is dizzy", "is charmed", "is distressed", "panics", "is afraid", "is enraged", "is frozen", "is shocked",
                                            "is poisoned", "", "", "", "", "is down"};
// A unit's ailments as they stand (EBtlDataBadStatus; empty = not said): on focus and in the details.
inline constexpr const char* kAilments[] = {"Dizzy", "Charmed", "Distressed", "Panicked", "Afraid", "Enraged", "Frozen", "Shocked",
                                            "Poisoned", "Orgia Mode", "Overheated", "Time bomb", "", "Downed"};
inline constexpr const char* kNoAilments = "No ailments";
inline constexpr const char* kNoEffects = "No buffs or debuffs";
inline constexpr const char* kGuarding = "Guarding";
inline constexpr const char* kUpTo = "up to";
inline constexpr const char* kTurn = "turn";
inline constexpr const char* kTurns = "turns";
inline constexpr const char* kPersonaWord = "Persona";
inline constexpr const char* kTheurgyCharge = "Theurgy gauge";
inline constexpr Hint kDetailKeys{"Control and the up and down arrows read the details of who is in focus.", "The right stick, up and down, reads the details of who is in focus."};
inline constexpr const char* kOneMore = "1 More!";
inline constexpr const char* kPersonaStatus = "Persona status";
inline constexpr const char* kLearnSkill = "Learn";
inline constexpr const char* kNewSkill = "New skill";
inline constexpr const char* kNextSkill = "Next skill";
inline constexpr const char* kLearned = "Learned";
inline constexpr Hint kCBack{"C goes back", "B goes back"};
inline constexpr const char* kEmptySlot = "Empty slot";
inline constexpr Hint kLearnSkillKeys{"Q and E switch Persona, Space learns. When asked to forget one, the arrows move over the slots, two columns of four; Escape goes back", "LB and RB switch Persona, A learns. When asked to forget one, the D-pad moves over the slots, two columns of four; B goes back"};
inline constexpr const char* kShuffleTime = "Shuffle Time";
inline constexpr Hint kShuffleKeys{"Left and right arrows choose a card, Space takes it.", "Left and right choose a card, A takes it."};
inline constexpr const char* kChoose = "choose";
inline constexpr const char* kMore = "more";                   // "choose 1 more"
inline constexpr const char* kArcanaBurst = "Arcana Burst";
inline constexpr const char* kPersonaOverstock = "Persona overstock. Choose a Persona to release";
inline constexpr const char* kGain = "Gain";                   // the overstock list's new Persona: "Gain: Apsaras, ..."
inline constexpr Hint kOverstockKeys{"Up and down arrows choose a Persona, Space releases it, F shows its stats.",
                                     "Up and down choose a Persona, A releases it, the Persona button shows its stats."};
inline constexpr Hint kSkillKeys{"Space confirms, Q and E change Persona, F Persona stats, Escape back.", "A confirms, LB and RB change Persona, X Persona stats, B back."};
inline constexpr Hint kKeyAttack{"Space attack", "A attack"};
inline constexpr Hint kKeySkills{"E skills", "Y skills"};
inline constexpr Hint kKeyItems{"R items", "X items"};
inline constexpr Hint kKeyGuard{"C guard", "B guard"};
inline constexpr Hint kKeyAnalyze{"Q analyze", "LB analyze"};
// The battle wheel's keys (battle.cpp, F1)
inline constexpr Hint kKeyTactics{"W tactics", "D-pad up tactics"};
inline constexpr Hint kKeySurvey{"S survey", "D-pad down survey"};
inline constexpr Hint kKeyRush{"Tab rush", "Menu button rush"};
inline constexpr Hint kKeyTheurgy{"T Theurgy", "RT Theurgy"};
inline constexpr Hint kKeyShift{"B shift", "LT shift"};
inline constexpr Hint kKeyAssist{"F assist", "RB assist"};
inline constexpr Hint kChangeTargetKeys{"A and D or the left and right arrows change target.", "Left and right change target."};
inline constexpr Hint kAnalyzeKeys{"A and D change target, Escape closes.", "LB and RB change target, B closes."};
inline constexpr Hint kTargetKeys{"Left and right arrows choose a target, Space confirms, Q analyzes it, Escape back.", "Left and right choose a target, A confirms, LB analyzes it, B back."};
inline constexpr Hint kTheurgyKeys{"Left and right arrows choose, Space uses it, Escape back.", "Left and right choose, A uses it, B back."};
inline constexpr Hint kShiftKeys{"Left and right arrows choose who takes the turn, Space passes it, F Persona stats, Escape back.", "Left and right choose who takes the turn, A passes it, X Persona stats, B back."};
inline constexpr Hint kSurveyKeys{"Up and down arrows go through everyone, Space analyzes a foe or opens a member's status, Escape back.", "Up and down go through everyone, A analyzes a foe or opens a member's status, B back."};
inline constexpr Hint kAllOutKeys{"Space to attack, Escape to hold back.", "A to attack, B to hold back."};
inline constexpr Hint kGuardKeys{"Space to guard, Escape to cancel.", "A to guard, B to cancel."};
inline constexpr const char* kAnalyze = "Analyze";
inline constexpr const char* kSurveyTitle = "Survey";
inline constexpr const char* kAllOutTitle = "All-Out Attack?";
inline constexpr const char* kGuardTitle = "Guard?";
inline constexpr const char* kTacticsTitle = "Tactics. Give an order?";
inline constexpr const char* kTacticsOrder = "Order";
inline constexpr const char* kTacticsEscape = "Escape";
inline constexpr const char* kTacticsWho = "Whose orders?";
inline constexpr const char* kTacticsAll = "Everyone";
inline constexpr const char* kTacticsWhich = "Which order?";
inline constexpr const char* kArcana[] = {"", "Fool", "Magician", "Priestess", "Empress", "Emperor", "Hierophant", "Lovers", "Chariot",
                                          "Justice", "Hermit", "Fortune", "Strength", "Hanged Man", "Death", "Temperance", "Devil",
                                          "Tower", "Star", "Moon", "Sun", "Judgement", "Aeon"};
inline constexpr const char* kSocialLinks = "Social Links";
inline constexpr const char* kNoSocialLinks = "No Social Links yet";
inline constexpr const char* kNewSocialLink = "New Social Link";
inline constexpr const char* kSocialLinkUp = "Social Link rank up";
inline constexpr const char* kSocialLinkMaxed = "Social Link at max rank";
inline constexpr const char* kSocialLink = "Social Link";
inline constexpr const char* kSocialLinkReversed = "Social Link reversed";
inline constexpr const char* kSocialLinkRestored = "Social Link restored";
inline constexpr const char* kRankMax = "MAX";
inline constexpr const char* kReversed = "reversed";
inline constexpr const char* kReadyToRankUp = "ready to rank up";
inline constexpr const char* kBondUp = "Bond deepened";
inline constexpr const char* kPoints = "points";
inline constexpr const char* kShop = "Shop";
inline constexpr const char* kBuy = "Buy";
inline constexpr const char* kQuit = "Quit";
inline constexpr const char* kEquip = "Equip";
inline constexpr const char* kDontEquip = "Don't Equip";
inline constexpr const char* kWeaponShopMenu[] = {"Buy Weapon", "Buy Armor", "Buy Footwear", "Buy Accessory", "Sell Item"};
inline constexpr const char* kOwned = "owned";
inline constexpr const char* kAntiqueShop = "Mayoido Antiques";
inline constexpr const char* kAntiqueMenu[] = {"Fuse Equipment", "Trade Card", "Trade Item"};
inline constexpr const char* kNeeds = "needs";
inline constexpr const char* kYouHave = "you have";
inline constexpr Hint kHowMany{"How many? W and S change, Space confirms.", "How many? Up and down change, A confirms."};
inline constexpr Hint kShopListKeys{"Up and down arrows choose, Space buys, Escape leaves.", "Up and down choose, A buys, B leaves."};
inline constexpr Hint kShopTopKeys{"Up and down arrows choose, Space opens, Escape leaves.", "Up and down choose, A opens, B leaves."};
inline constexpr Hint kShopWeaponKeys{"Up and down arrows choose, Space buys, Q and E switch member, Escape back.", "Up and down choose, A buys, LB and RB switch member, B back."};
inline constexpr Hint kShopSellKeys{"Up and down arrows choose, Space sells, Q and E switch category, Escape back.", "Up and down choose, A sells, LB and RB switch category, B back."};
inline constexpr Hint kShopTradeKeys{"Up and down arrows choose, Space trades, Escape back.", "Up and down choose, A trades, B back."};
inline constexpr Hint kShopYesNoKeys{"Left and right arrows choose, Space confirms.", "Left and right choose, A confirms."};
inline constexpr const char* kAttack = "attack";
inline constexpr const char* kAccuracy = "accuracy";
inline constexpr const char* kDefence = "defense";
inline constexpr const char* kEvasion = "evasion";
inline constexpr const char* kSystemItems[] = {"Tutorial", "Config", "Dictionary", "Load Data", "Save Data", "Rewind", "Return to Title"};
inline constexpr const char* kTutorials = "Tutorials";
inline constexpr const char* kTutorialTabs[5] = {"Battle", "Dungeon", "Daily Life", "Fusion", "System"};   // drawn as pictures
inline constexpr const char* kNoMessage = "No message";
inline constexpr const char* kAKey = "a key";
inline constexpr Hint kYesNoHint{"Confirm for yes, cancel for no.", "A for yes, B for no."};
inline constexpr const char* kThinking = "Thinking";
inline constexpr Hint kTutorialKeys{"Q and E turn the pages, Space goes on.", "LB and RB turn the pages, A goes on."};
inline constexpr Hint kDialogueKeys{"Conversation keys: Space next, B log, Shift auto, Tab fast-forwards.",
                                    "Conversation keys: A next, L3 log, X auto, Menu button fast-forwards."};
inline constexpr Hint kFieldKeys{"Field keys: Space checks and talks, Tab menu, M map, B log, Ctrl messages or attack, Shift dash, H saves.",
                                 "Field keys: A checks and talks, Y menu, LB map, L3 log, X messages or attack, RT dash, Menu button saves."};
// Added to the field's keys on a Tartarus floor. Auto-Recover's button is the one its tutorial draws (icon 117).
inline constexpr Hint kTartarusKeys{"In Tartarus: Y Auto-Recover, N Navi.", "In Tartarus: RB Auto-Recover, View Navi."};
// The help keys (F1 = this screen's keys, Shift+F1 = the mod's own keys, F2 its settings; on a controller, the mod
// layer's Menu, and View for the settings, whose last row reads the keys).
inline constexpr Hint kModKeysHint{"Shift+F1 lists the mod's keys, F2 opens its settings.", "Both triggers and View open the mod's settings; its last row lists the mod's keys."};
inline constexpr const char* kNoKeys = "No keys to tell here.";
inline constexpr const char* kModKeys = "Mod keys";
inline constexpr const char* kPadLayer = "Hold both triggers, then";
// The mod settings menu (settings.cpp)
inline constexpr const char* kModSettings = "Mod settings";
inline constexpr const char* kModSettingsClosed = "Mod settings closed";
inline constexpr Hint kModSettingsKeys{"Up and down arrows choose, letters jump to a setting, left and right arrows change, Space changes or reads, Escape closes.",
                                       "Up and down choose, left and right change, A changes or reads, B closes."};
inline constexpr const char* kSetWallTones = "Wall tones";
inline constexpr const char* kSetBeacon = "Beacon";
inline constexpr const char* kSetNearbyChimes = "Nearby chimes";
inline constexpr const char* kSetShadowAlerts = "Shadow alerts";
inline constexpr const char* kSetHrtf = "3D sound for headphones";
inline constexpr const char* kSetBehindPitch = "Lower pitch behind you";
inline constexpr const char* kSetPoiRange = "Points of interest range";
inline constexpr const char* kSetMasterVolume = "Mod master volume";
inline constexpr const char* kSetSoundVolume = "Beeps and chimes volume";
inline constexpr const char* kSetWallVolume = "Wall tone volume";
inline constexpr const char* kSetTriggerDelay = "Controller trigger delay";
inline constexpr const char* kSetEntryHints = "Menu key hints on entry";
inline constexpr const char* kSetVoicedLines = "Voiced dialogue";
inline constexpr const char* kVoicedRead = "read aloud";
inline constexpr const char* kVoicedName = "speaker's name only";
inline constexpr const char* kVoicedSilent = "not read, the voice plays alone";
inline constexpr const char* kSetVoiceEndSound = "When a line ends";
inline constexpr const char* kSetSubtitles = "Read cutscene subtitles";
inline constexpr const char* kSetAudioDescription = "Audio description in movies";
inline constexpr const char* kSetDescriptionVolume = "Audio description volume";
inline constexpr const char* kEndNext = "say Next";
inline constexpr const char* kNext = "Next";
inline constexpr const char* kAutoOn = "Auto on";
inline constexpr const char* kAutoOff = "Auto off";
inline constexpr const char* kFastForwardOn = "Fast-forward on";
inline constexpr const char* kFastForwardOff = "Fast-forward off";
inline constexpr const char* kEndPage = "page turn sound";
inline constexpr const char* kEndMessage = "message chime sound";
inline constexpr const char* kEndCursor = "cursor tick sound";
inline constexpr const char* kMetersUnit = "meters";
inline constexpr const char* kPercent = "percent";
inline constexpr const char* kMilliseconds = "milliseconds";
inline constexpr Hint kListKeys{"Up and down arrows choose, Space confirms, Escape back.", "Up and down choose, A confirms, B back."};
inline constexpr Hint kTutorialListKeys{"Up and down arrows choose, Q and E switch tabs, Space opens, Escape back.", "Up and down choose, LB and RB switch tabs, A opens, B back."};
inline constexpr Hint kNoticeKeys{"Space continues.", "A continues."};
inline constexpr Hint kChooseKeys{"Up and down arrows choose, Space confirms.", "Up and down choose, A confirms."};
inline constexpr Hint kCampKeys{"Space opens, Escape closes.", "A opens, B closes."};
// Points of interest
// A way into another area: said by its name alone, which is where it leads ("Gekkoukan High School exit" was taken
// for the way out of the school; the players' call, 2026-11-09). This is the one with no name.
inline constexpr const char* kPoiExit = "Exit";
inline constexpr const char* kVerbGoTo = "Go to";
inline constexpr const char* kVerbTalk = "Talk";
inline constexpr const char* kVerbListen = "Listen";
inline constexpr const char* kVerbShop = "Shop";
inline constexpr const char* kVerbAction = "Action";
inline constexpr const char* kVerbCheck = "Check";
inline constexpr const char* kPoiPerson = "Person";
inline constexpr const char* kPoiShop = "Shop";
inline constexpr const char* kPoiDoor = "Door";
inline constexpr const char* kPoiThing = "Object";
inline constexpr const char* kPoiEvent = "Event";
inline constexpr const char* kPoiShadow = "Shadow";
inline constexpr const char* kPoiBreakable = "Breakable object";
inline constexpr const char* kCatExits = "Exits";
inline constexpr const char* kCatPeople = "People";
inline constexpr const char* kCatShops = "Shops";
inline constexpr const char* kCatDoors = "Doors";
inline constexpr const char* kCatThings = "Objects";
inline constexpr const char* kCatShadows = "Shadows";
inline constexpr const char* kShadowBackTurned = "Shadow close, back turned";
inline constexpr const char* kShadowFacingYou = "Shadow close, facing you";
inline constexpr const char* kNothingNearbyFull = "Nothing nearby";
inline constexpr const char* kNothingSelected = "Nothing selected";
// Navigation
inline constexpr const char* kNoPath = "No path";
inline constexpr const char* kStuck = "Stuck, stopping";
inline constexpr const char* kStopped = "Stopped";
inline constexpr const char* kNotInField = "Not in the field";
inline constexpr const char* kBeaconOn = "Beacon on";
inline constexpr const char* kBeaconOff = "Beacon off";
inline constexpr const char* kObjective = "Objective";
inline constexpr const char* kNoObjective = "No objective shown yet";
inline constexpr const char* kNothingToReview = "No menu or message to read";
inline constexpr const char* kNoAreaDescription = "No description for this place:";
inline constexpr const char* kFloor = "floor";
inline constexpr const char* kTartarus = "Tartarus";
inline constexpr const char* kUnknownArea = "Unknown area";
inline constexpr const char* kWallTonesOn = "Wall tones on";
inline constexpr const char* kWallTonesOff = "Wall tones off";
// Velvet Room (the menus are pictures: the mod's own words)
inline constexpr const char* kVelvetMenu[] = {"Fusion", "Summon from the Compendium", "Register to the Compendium"};
inline constexpr const char* kVelvetTop = "What do you wish for?";
inline constexpr const char* kFusionMethods[] = {"Dyad Fusion, two Personas", "Special Fusion", "Fusion Search"};
inline constexpr const char* kFusionMethod = "Choose a fusion method.";
inline constexpr const char* kFusionFirst = "Choose the base Persona.";
inline constexpr const char* kFusionSecond = "Choose the second Persona.";
inline constexpr const char* kFusionSearch = "Fusion Search. Choose a Persona to fuse.";
inline constexpr const char* kFusionFrom = "from";
inline constexpr const char* kFusionAnd = "and";
inline constexpr Hint kFusionSearchKeys{"Space confirms, Q and E sort, Escape back.", "A confirms, LB and RB sort, B back."};
inline constexpr const char* kFusionNone = "No Persona can be fused.";
inline constexpr const char* kSpecialFusion = "Special Fusion. Choose a Persona to fuse.";
inline constexpr const char* kSpecialHave = "you have";
inline constexpr const char* kSpecialReady = "ready to fuse";
inline constexpr const char* kCompendium = "Compendium. Select a Persona.";
inline constexpr const char* kNotRegistered = "not registered";
inline constexpr const char* kRegister = "Who shall you register?";
inline constexpr const char* kRegisterThis = "Register this Persona?";
inline constexpr const char* kSummonThis = "Summon this Persona?";
inline constexpr Hint kStatusConfirmKeys{"Space registers it, Page Down and Page Up jump between sections, F skills and affinities, Tab registered stats, Escape back.", "A registers it, RT and LT jump between sections, X skills and affinities, Y registered stats, B back."};
inline constexpr Hint kRegisterKeys{"Up and down arrows choose, Space opens its status, Tab marks it, F registers the marked ones or all, Escape back.", "Up and down choose, A opens its status, Y marks it, X registers the marked ones or all, B back."};
inline constexpr Hint kCompendiumPageKeys{"Space summons, Tab and Shift Tab jump between sections, F skills and affinities, Escape back.", "A summons, RT and LT jump between sections, X skills and affinities, B back."};
inline constexpr Hint kLevelUpKeys{"Tab and Shift Tab jump between sections, Space continues.", "RT and LT jump between sections, A continues."};
inline constexpr Hint kCompendiumKeys{"Space summons, Q and E sort, Tab registered only, F lock, Escape back.", "A summons, LB and RB sort, Y registered only, X lock, B back."};
inline constexpr const char* kFusionGives = "makes";
inline constexpr const char* kFusionImpossible = "cannot fuse";
inline constexpr const char* kAboveYourLevel = "above your level";
inline constexpr const char* kAlreadyOwned = "already in your stock";
inline constexpr Hint kFusionKeys{"Space confirms, F Persona stats, Escape back.", "A confirms, X Persona stats, B back."};
inline constexpr const char* kEquipped = "equipped";
inline constexpr const char* kFusionResult = "Result";
inline constexpr const char* kFusionMaterial = "Material";
// Persona status pages (camp, Velvet Room, battle)
inline constexpr const char* kStatNames[] = {"Strength", "Magic", "Endurance", "Agility", "Luck"};
inline constexpr const char* kStatsTitle = "Stats";
inline constexpr const char* kAffinitiesTitle = "Affinities";
inline constexpr const char* kNoAffinities = "no weaknesses or resistances";
inline constexpr const char* kLearnsTitle = "Learns later";
inline constexpr const char* kLearnsAt = "at level";
inline constexpr const char* kSlBonus = "Social Link bonus";
inline constexpr const char* kSlRank = "rank";
inline constexpr const char* kExpToNext = "EXP to next level";
inline constexpr const char* kRaisesTo = "up to level";
inline constexpr const char* kUp = "up";
inline constexpr const char* kNew = "new";
inline constexpr const char* kInheritTitle = "Choose skills to inherit.";
inline constexpr const char* kInherits = "Inherits";
inline constexpr const char* kNotChosen = "not chosen";
inline constexpr const char* kFusing = "Fusing.";
inline constexpr Hint kFusingKeys{"Tab skips the animation.", "The game's skip button (Tab on the keyboard) skips the animation."};
inline constexpr Hint kInheritSkill{"Up and down arrows move, Space takes a skill or puts it back, Escape back.", "Up and down move, A takes a skill or puts it back, B back."};
inline constexpr Hint kFusionConfirmKeys{"Space fuses, Tab and Shift Tab jump between sections, Q and E page through the result and its materials, F skills and affinities, Escape back.", "A fuses, RT and LT jump between sections, LB and RB page through the result and its materials, X skills and affinities, B back."};
// Config (System > Config, and the title screen's)
inline constexpr const char* kConfig = "Config";
inline constexpr const char* kOn = "on";
inline constexpr const char* kOff = "off";
inline constexpr const char* kUnavailable = "unavailable";
inline constexpr const char* kUnsupportedBuild = "This copy of Persona 3 Reload is not the version Persona 3 Access was made for, such as the demo or an older update. To keep the game from freezing, menus are not read. Messages and the field still are.";
inline constexpr Hint kConfigTabKeys{"Up and down arrows choose, Space opens, Escape back. F2 opens the mod's settings.", "Up and down choose, A opens, B back. Both triggers and View open the mod's settings."};
inline constexpr Hint kBgmKeys{"Up and down arrows choose, Space sets it, F plays it, Escape back.", "Up and down choose, A sets it, X plays it, B back."};
inline constexpr Hint kConfigKeys{"Up and down arrows choose, left and right arrows change, Space saves changes, H restores defaults, Escape back. F2 opens the mod's settings.", "Up and down choose, left and right change, A saves changes, Menu button restores defaults, B back. Both triggers and View open the mod's settings."};
inline constexpr const char* kKeyCode = "key code";
inline constexpr const char* kPlaylist = "Playlist. What do you want to change it to?";
inline constexpr const char* kFavorite = "favorite";
inline constexpr const char* kDictionary = "Dictionary";
inline constexpr const char* kNoParty = "No party";
inline constexpr const char* kTownInfo = "Places here";
inline constexpr const char* kTheurgy = "Theurgy";
inline constexpr const char* kBuffs[] = {"attack up", "attack down", "defense up", "defense down", "agility up", "agility down"};
inline constexpr const char* kCharged = "Charged";
inline constexpr const char* kConcentrating = "Concentrating";
inline constexpr const char* kPhysicalShield = "Physical attacks reflected";
inline constexpr const char* kMagicShield = "Magic attacks reflected";
inline constexpr const char* kAilmentsDoubled = "Ailments land more easily";
inline constexpr const char* kWalls[] = {"Fire resisted", "Ice resisted", "Electricity resisted", "Wind resisted"};
inline constexpr const char* kBreaks[] = {"Fire resistance broken", "Ice resistance broken", "Wind resistance broken", "Electricity resistance broken"};
inline constexpr Hint kTheurgyReady{"Theurgy ready, T", "Theurgy ready, RT"};
inline constexpr const char* kLog = "Log";
inline constexpr const char* kRequests = "Elizabeth's requests";
inline constexpr const char* kMissingPersons = "Missing Persons";
inline constexpr const char* kNumber = "Number";
inline constexpr const char* kUnknown = "unknown";
inline constexpr const char* kMissingFound = "found";
inline constexpr const char* kMissingDiscontinued = "discontinued";
inline constexpr const char* kMissingSearching = "searching";
inline constexpr Hint kRequestKeys{"Up and down arrows choose, Space accepts or reports at the Velvet Room, F info, X sorts, Escape closes", "Up and down choose, A accepts or reports at the Velvet Room, X info, RT sorts, B closes"};
inline constexpr Hint kPersonaStatusKeys{"Tab and Shift Tab jump between sections, Q and E switch Persona, Escape back", "RT and LT jump between sections, LB and RB switch Persona, B back"};
inline constexpr const char* kTacticsHeading = "Tactics";
inline constexpr const char* kTerminal = "Terminal. Where do you want to go?";
inline constexpr Hint kTerminalKeys{"Up and down arrows choose, Space travels, Escape closes", "Up and down choose, A travels, B closes"};
inline constexpr const char* kEntrance = "Entrance";
inline constexpr const char* kFloorTitle = "Floor";
inline constexpr const char* kYouAreHere = "furthest floor reached";
inline constexpr Hint kSelectKeys{"Up and down arrows choose, Space confirms, Escape closes", "Up and down choose, A confirms, B closes"};
inline constexpr Hint kLogKeys{"Up and down arrows scroll, Space replays the voice, Escape closes", "Up and down scroll, A replays the voice, B closes"};
inline constexpr const char* kLogEarlier = "An earlier line, from before this session";
inline constexpr const char* kShift = "Shift. Pass the turn to";
inline constexpr Hint kShiftAvailable{"B shifts the turn to another member", "LT shifts the turn to another member"};
inline constexpr const char* kRewind = "Rewind. Choose a day to go back to";
// The menu navigator (core/navigator.cpp)
inline constexpr const char* kNoTooltip = "no details";
inline constexpr const char* kNothingToCompare = "nothing to compare";
inline constexpr const char* kNoDetails = "no details";
inline constexpr const char* kNoMatch = "no match for";
inline constexpr const char* kSearchCleared = "search cleared";
inline constexpr const char* kExpanded = "expanded";
inline constexpr const char* kCard = "Card";
// Shuffle Time's Minor Arcana suits, said with the card's number: "Wands 1".
inline constexpr const char* kSuitSwords = "Swords";
inline constexpr const char* kSuitCoins = "Coins";
inline constexpr const char* kSuitCups = "Cups";
inline constexpr const char* kSuitWands = "Wands";
inline constexpr const char* kCollapsed = "collapsed";
// Camp submenus (camp_menus.cpp)
// Their keys (F1), from each menu's native input function: Q / E LB / RB,
// F the X button, X RT, Z LT, H the Menu button.
inline constexpr Hint kItemListKeys{"Up and down arrows choose, Q and E switch category, Space uses, Escape back.", "Up and down choose, LB and RB switch category, A uses, B back."};
inline constexpr Hint kEquipSlotKeys{"Up and down arrows choose a slot, Space changes it, Q and E switch member, Escape back.", "Up and down choose a slot, A changes it, LB and RB switch member, B back."};
inline constexpr Hint kEquipGearKeys{"Up and down arrows choose, Space equips, Q and E switch member, F reads it again, Escape back.", "Up and down choose, A equips, LB and RB switch member, X reads it again, B back."};
inline constexpr Hint kCampPersonaKeys{"Tab and Shift Tab jump between sections, Q and E switch Persona, Space on its name equips it, H releases, Escape back.", "RT and LT jump between sections, LB and RB switch Persona, A on its name equips it, Menu button releases, B back."};
inline constexpr Hint kCampPersonaStatusKeys{"Tab and Shift Tab jump between sections, Q and E switch Persona, Space equips it, Escape back.", "RT and LT jump between sections, LB and RB switch Persona, A equips it, B back."};
inline constexpr Hint kStatsKeys{"Up and down arrows choose, Space opens their page, Q and E tactics, F joins or leaves the party, Escape back.", "Up and down choose, A opens their page, LB and RB tactics, X joins or leaves the party, B back."};
inline constexpr Hint kTacticsListKeys{"Up and down arrows choose, Space changes their tactic, Q and E back to stats, Escape back.", "Up and down choose, A changes their tactic, LB and RB back to stats, B back."};
// The triggers are the game's on this page (Theurgy, Persona status): on a pad the D-pad walks its lines.
inline constexpr Hint kStatsPageKeys{"Tab and Shift Tab jump between sections, Q and E switch member, X Persona status, Z Theurgy, F joins or leaves the party, Escape back.", "Up and down go through the lines, LB and RB switch member, RT Persona status, LT Theurgy, X joins or leaves the party, B back."};
inline constexpr Hint kCampRequestKeys{"Up and down arrows choose, F info, X sorts, Q and E switch tabs, Escape closes.", "Up and down choose, X info, RT sorts, LB and RB switch tabs, B closes."};
inline constexpr Hint kMissingPersonsKeys{"Up and down arrows choose, F reads it again, X sorts, Q and E switch tabs, Escape closes.", "Up and down choose, X reads it again, RT sorts, LB and RB switch tabs, B closes."};
inline constexpr Hint kMissingBoardKeys{"Up and down arrows choose, F reads it again, X sorts, Escape closes.", "Up and down choose, X reads it again, RT sorts, B closes."};
inline constexpr Hint kCalendarKeys{"Left and right arrows move a day, up and down arrows a week, Q and E change month, F reads the day's plans, Escape closes.", "Left and right move a day, up and down a week, LB and RB change month, X reads the day's plans, B closes."};
inline constexpr Hint kCalendarPlansKeys{"Up and down arrows go through the plans, F or Escape closes them.", "Up and down go through the plans, X or B closes them."};
inline constexpr Hint kSocialPageKeys{"Up and down arrows choose, Q and E switch link, Escape back.", "Up and down choose, LB and RB switch link, B back."};
inline constexpr Hint kTheurgyListKeys{"Left and right arrows choose, Q and E switch character, F personality, Escape back.", "Left and right choose, LB and RB switch character, X personality, B back."};
inline constexpr const char* kItemCategories[] = {"Order Obtained", "Consumable Items", "Skill Cards", "Materials & Valuables", "Event & Misc."};
inline constexpr const char* kItem = "Item";
inline constexpr const char* kNoItems = "No items";
inline constexpr const char* kUseOnWhom = "On whom?";
inline constexpr const char* kWhichPersona = "Which Persona?";
inline constexpr const char* kEquipSlots[] = {"Weapon", "Armor", "Footwear", "Accessory", "Outfit"};
inline constexpr const char* kTactic = "Tactic";
inline constexpr const char* kChosen = "chosen";
inline constexpr const char* kTheurgyObtained = "New Theurgy";
inline constexpr const char* kTheurgyGauge = "Gauge condition";
inline constexpr const char* kNotEnough = "not enough";
// Shops (shop.cpp)
inline constexpr const char* kOption = "Option";
inline constexpr const char* kCannotBuy = "not now";
inline constexpr const char* kForWhom = "For whom?";
inline constexpr const char* kFor = "for";
inline constexpr const char* kSell = "Sell";
inline constexpr const char* kSellAll = "Sell all items";
inline constexpr const char* kWhichMaterial = "Which material do you want to use?";
inline constexpr const char* kBuyQuestion = "Is this what you want?";
inline constexpr const char* kEquipQuestion = "Equip it now?";
inline constexpr const char* kSellConfirm = "Sell it?";
inline constexpr const char* kSellOldQuestion = "Sell what they wore before?";
inline constexpr const char* kKeep = "Keep";
inline constexpr const char* kMarked = "marked";
inline constexpr const char* kNoTutorials = "There are no tutorials to display.";
// The camp's tactics as the game draws them (Stats, E), by the unit's tactic index (unit +0x296; read 2026-10-30)
inline constexpr const char* kTacticNames[] = {"Direct Commands", "Act Freely", "Full Assault", "Conserve SP", "Heal/Support"};
inline constexpr const char* kRequestStatus[] = {"not accepted", "accepted", "completed", "expired", "ready to report"};
// Story cut-ins (cutin.cpp): the close-up a scene flashes before a line it stresses, by the game's character id and
// picture (T_UI_CI_<PC|SC><char>_<pic>, each picture looked at; 2026-11-02).
struct Cutin { int chara, pic; const char* text; };
inline constexpr Cutin kCutins[] = {
  {2, 1, "Close-up: Yukari, smiling."},
  {2, 2, "Close-up: Yukari, glaring."},
  {2, 4, "Close-up: Yukari, wide-eyed in surprise."},
  {2, 10, "Close-up: Yukari, blushing, eyes squeezed shut."},
  {3, 1, "Close-up: Junpei, grinning."},
  {3, 2, "Close-up: Junpei, scowling."},
  {3, 4, "Close-up: Junpei, eyebrows raised in surprise."},
  {3, 10, "Close-up: Junpei in the dark, terrified, teeth clenched."},
  {3, 11, "Close-up: Junpei, cap on backwards, grinning."},
  {3, 12, "Close-up: Junpei, cap on backwards, fired up, teeth clenched."},
  {4, 1, "Close-up: Akihiko, smirking."},
  {4, 2, "Close-up: Akihiko, frowning hard."},
  {4, 4, "Close-up: Akihiko, wide-eyed."},
  {4, 10, "Close-up: Akihiko, sweating nervously."},
  {5, 1, "Close-up: Mitsuru, eyes closed, a faint smile."},
  {5, 2, "Close-up: Mitsuru, a cold, stern stare."},
  {5, 4, "Close-up: Mitsuru, startled."},
  {5, 10, "Close-up: Mitsuru, hair tied up, blushing."},
  {6, 1, "Close-up: Fuuka, smiling gently."},
  {6, 2, "Close-up: Fuuka, determined."},
  {6, 4, "Close-up: Fuuka, wide-eyed in alarm."},
  {7, 2, "Close-up: Aigis, wide-eyed and intent."},
  {8, 1, "Close-up: Ken, a small smile."},
  {8, 2, "Close-up: Ken, frowning."},
  {8, 4, "Close-up: Ken, startled."},
  {9, 1, "Close-up: Koromaru, happy, tongue out."},
  {9, 2, "Close-up: Koromaru, snarling, eyes red."},
  {10, 2, "Close-up: Shinjiro, glaring from under his beanie."},
  {11, 1, "Close-up: Metis, a soft smile."},
  {11, 2, "Close-up: Metis, glaring."},
  {11, 4, "Close-up: Metis, startled."},
  {12, 1, "Close-up: Aigis, smiling."},
  {12, 2, "Close-up: Aigis, determined."},
  {12, 10, "Close-up: Aigis, wide-eyed, lips parted."},
  {211, 10, "Close-up: Takaya, a menacing grin."},
  {212, 10, "Close-up: Jin, glaring through his glasses."},
  {213, 10, "Close-up: Chidori, a faint smile."},
  {221, 10, "Close-up: Ryoji, smiling gently."},
  {222, 10, "Close-up: Ikutsuki, a wide, manic grin."},
  {222, 11, "Close-up: Ikutsuki, smiling pleasantly."},
  {223, 10, "Close-up: a student with a ponytail, panicked, teeth clenched."},
};
// Pictures a scene shows full screen (picture.cpp, AUIMiscPictureDraw; T_UI_Picture_<id>, each looked at 2026-11-03):
// the chalkboard picture of a class question (UI/Picture/Lesson; never the answer, the teacher names it after) and
// the story's sketch (UI/Picture/Event). Tutorial/T_UI_Picture_200 is an empty placeholder.
struct ScenePicture { const char* texture; const char* text; };
inline constexpr ScenePicture kScenePictures[] = {
  {"T_UI_Picture_000", "Picture: a pencil sketch in a spiral-bound sketchbook: Junpei in his baseball cap and a high-collared jacket, a little goatee on his chin, smiling as he looks off to the side."},
  {"T_UI_Picture_100", "Picture: four spirals, each drawn around the origin of x and y axes. A: a spiral that widens fast, each turn far wider than the one inside it, ending in a broad open sweep. B: evenly spaced rings, every turn the same distance from the last. C: rings that crowd together toward the outside, around a small open curl at the centre. D: a few tight turns around the origin that unwind into a long tail, flattening out to the right along the top."},
  {"T_UI_Picture_101", "Picture: the same four spirals, with A, the one that widens fast, boxed in red."},
  {"T_UI_Picture_102", "Picture: a device on a flat base: two long arms hinged at their middles form a tall rhombus, with a small folding arm inside it, and a contact strip across the top."},
  {"T_UI_Picture_103", "Picture: on x and y axes, a circle resting on the origin, touching a horizontal line across its top. A line from the origin through the circle runs out to that top line; a red bell-shaped curve peaks at the top of the circle and falls away on both sides toward the x axis."},
};
}  // namespace p3a::strings
