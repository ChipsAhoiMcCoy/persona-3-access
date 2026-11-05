# Menu coverage (bespoke menus, branch ui-revamp, 2026-10-30)

Every interactive menu of the base game, the mod screen that models it (src/screens/), and how it was verified in the
dev game. "Live" = navigated in the running game with the game's own highlight following the mod's focus.

| Game menu | Game object | Mod screen | Verified |
|---|---|---|---|
| Title menu | ATitleActor | title_menu (title.cpp) | live |
| Difficulty | DifficultySelectionActor | difficulty (title.cpp) | live |
| Contract (name entry: the two fields once both are typed) | ANameEntry | name_entry | live (keyboard; pad A and D-pad) |
| Mod settings (F2) | the mod's own | settings.cpp on core/navigator | live |
| SELECT lists (fortune, garden, Igor's Talk, game over) | UGenericSelect* | generic_select | live |
| Dialogue choices (incl. TV shop, Yes / No prompts) | USelItem | choice | live |
| Camp root | UCmpRoot | camp | live |
| Camp Item (list, member, Persona) | UCmpItem | camp_item | live |
| Camp Skill (member, skill, target) | UCmpSkill | camp_skill | live |
| Camp Equip (member, slot, gear) | UCmpEquip | camp_equip | live |
| Camp Persona | UCmpPersona | camp_persona | live |
| Persona status page (camp, Velvet Room, battle) | APersonaStatus | persona_status, battle_persona | live |
| Stats list, Tactics, orders | UCmpStatus | camp_stats | live |
| Stats member page | CmpCharacterStatus | camp_stats_page | live |
| Social Link list | UCmpCommu | camp_social | live |
| Social Link page (memo, people) | CmpCommuDetails | camp_social_details | live |
| Quest: Elizabeth's requests (camp and her counter) | UIRequest | camp_quest | live |
| Quest: Missing Persons (camp and board) | UIMissingPerson | camp_quest | live (camp) |
| Calendar (day grid, plans) | UCmpCalendar | camp_calendar | live |
| System | UCmpSystem | camp_system | live |
| Tutorials | AUITutorial | tutorial_list | live |
| Config (tabs, rows, values, BGM playlist) | AUIConfiguration | config | live |
| Dictionary | AUIDictionary | dictionary | live |
| Save / load / Rewind | AUISaveLoad | save_load | live |
| Mail (list, reading) | AMailActor | mail | live (a delivered text) |
| Log | AUIBackLog | backlog | live |
| Town map | AUITownMapActor | town_map | live |
| "Where should I go?" (M key, entrances) | UUILocationSelect | location_list | live |
| Tartarus terminal | AUIDungeonTransfer | terminal | live |
| Shops: weapon, item, sell, quantity, prompts | AFclShopBase | shop | live |
| Antique shop: fusion, trade item, trade card | AFclAntiqueShop | shop | live |
| Velvet Room: top, method, dyad, special, search | AUIVelvetRoom, UICombine | velvet | live |
| Velvet Room: inheritance, compendium, registration | APersonaStatus, UICompleteBook(Regist) | velvet | live |
| Skill learning (forget a slot) | APersonaStatus sequences | skill_learn | live |
| Theurgy obtained (Social Link event) | UITheurgia | theurgy (background) | live |
| Theurgy list (Z on a Stats member's page) and personality (F) | UITheurgia mode 1 | theurgy | live |
| Battle command list | BP_BtlTopUI_C | battle_commands | live |
| Battle skills, items, targets, Theurgy | BP_BtlSkillList_C etc. | battle_* | live |
| Battle Tactics | BP_BtlStrategyInstruct_C | battle_tactics | live |
| Battle Shift | BP_BtlGuiShiftSelect_C | battle_shift | live (Shift to Junpei) |
| Battle Survey | BP_BtlTacticsCheck_C | battle_survey (follow-only) | live |
| All-Out Attack / Guard prompts | BtlGuiStateManager 5 / 11 | battle (said on entry; the game's keys answer) | live |
| Shuffle Time | BtlShuffleMainBase | shuffle | live |

Not menus, left as readers: notices (item get, stats up, rank up, victory pages, day change), movies, credits, the
area map; the contract's typing (a text field: the game has the keys, the mod echoes). The Theurgy list is not Episode Aigis only: it opens from Stats in the
base game once a party member knows a Theurgy (gate UUICmpStatus +0x58).


