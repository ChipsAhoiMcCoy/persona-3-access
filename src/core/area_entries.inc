// The area descriptions (included by area_text.cpp). One Entry per area, or per storey of a multi-storey area.
// What each place holds comes from the game's own data: its named objects (DT_FldDailyHitName rows placed in the
// area's Hit level, with their heights per storey), its prop and variant sublevels (LV_F<major>_<minor>_BG_*), the
// light colours of Tartarus's block parts, and screenshots of the places (2026-11-02: the dorm, the school, the town
// by day and night, Adamah). Field ids: LV_F<major>_<minor>. Episode Aigis' copies (122 dorm, 123 mall, 401
// Tartarus) share these.
const Entry kEntries[] = {
// ---- Gekkoukan High School (F101) ----
{.major = 101, .minor = 101, .outdoor = true,
 .base = "School Entranceway. The front gate stands between white marble gateposts with ornate wrought-iron gates, and a "
         "wide paved approach lined with trees leads to Gekkoukan High. Broad stone steps climb to its main entrance: a "
         "colonnade of massive round pillars standing before a tall facade of glass and dark steel. Through the glass "
         "you can make out the entrance hall and its rows of shoe lockers. Trees line the approach, and students stream "
         "up and down the steps every morning and after class.",
 .day = "Sunlight glints off the great glass facade.",
 .evening = "The low sun throws the pillars' long shadows across the steps, and the building glows warm gold.",
 .night = "The school stands dark and locked, its glass facade black.",
 .spring = "The cherry trees along the approach are in full bloom, and pink petals drift down over the steps.",
 .summer = "The trees are thick with green leaves, throwing patches of shade over the hot stone.",
 .autumn = "The trees have turned red and gold, and fallen leaves skitter across the steps.",
 .winter = "The trees are bare, and the stone approach looks grey and wind-swept under the cold sky."},
{.major = 101, .minor = 102, .floor = 1,
 .base = "School Building, first floor: the main lobby. A grand, high-ceilinged hall like an old bank, held up by "
         "massive fluted stone columns, with a polished tile floor and an ornate inlaid pattern around a statue of a "
         "figure holding a sword and a staff. A wide staircase rises under an arch at the back, and tall latticed "
         "windows glow along one side. A counter sells school goods, and bulletin boards carry event notices and, at "
         "exam time, the posted results. Hallways lead off to the faculty office, the nurse's office, the library, the "
         "laboratory, the science, art, music and home economics rooms with their prep rooms, the printing room, the "
         "janitor's room and the restrooms."},
{.major = 101, .minor = 102, .floor = 2,
 .base = "School Building, second floor: the classroom hallway. A long corridor of pale stone tiles laid in a diamond "
         "pattern, lined with sliding classroom doors set with frosted glass, transom windows above them, and cork "
         "bulletin boards crowded with flyers. Classroom 2-E is here, and your own classroom 2-F, with the computer "
         "room, the PA room and the student council room further along, and restrooms by the stairs. Students linger "
         "and chat in the hall between classes. Stairs lead down to the lobby and up toward the third floor and the roof."},
{.major = 101, .minor = 103, .outdoor = true,
 .base = "The outdoor corridor behind the school building: an open-air arcade of weathered stone-brick arches, lamps "
         "fixed to its pillars, with a tiled walkway running between them to the double doors of the gym building. "
         "Small gardens of grass and shrubs lie on either side, a young tree stands alone by the path, a whiteboard holds "
         "club notices, and a locked gate closes off the far side. The gym, the indoor swimming pool and the practice "
         "field are all reached from here.",
 .evening = "Shouts and whistles drift over from the practice field, and the lamps glow against the orange evening.",
 .spring = "", .autumn = "Fallen leaves gather in the corners of the arcade."},
{.major = 101, .minor = 121,
 .base = "Classroom 2-F, your homeroom. Rows of white desks and red chairs stand on a checkered tile floor, facing the "
         "blackboard and the teacher's lectern; low cubby shelves for bags line the back wall. A wall of tall windows "
         "hung with yellow curtains looks out over the trees of the school grounds to another wing of the school with "
         "arched windows.",
 .day = "Sunlight pours through the windows across the desks.",
 .evening = "The room is emptying, and orange evening light floods in through the windows, laying the window frames' "
            "shadows across the desks.",
 .night = "The room is dark and empty, the desks in silent rows.",
 .spring = "Outside the windows the cherry trees are in bloom.",
 .summer = "Outside the windows the trees are deep summer green.",
 .autumn = "Outside the windows the trees have turned red and gold.",
 .winter = "Outside the windows the trees stand bare."},
{.major = 101, .minor = 141, .outdoor = true,
 .base = "The school rooftop. A broad concrete deck ringed by a metal railing, with stone benches and flower planters "
         "along the edges. The stairwell housing has double doors with pale blue glass, and a water tank and antennas on "
         "its roof; there is a small storage room too. A tall white wind turbine turns slowly beside the building. "
         "Beyond the railing lies the sea and the rooftops of Port Island. Students come here to eat lunch or talk "
         "away from the crowds.",
 .day = "The wind is fresh up here, and the sky is wide and open.",
 .evening = "The sun sets over the sea under pink and violet clouds, and the whole roof turns gold.",
 .night = "The city lights glitter below, and the sky above is dark and wide.",
 .spring = "A mild spring breeze carries the scent of the flower planters.",
 .summer = "The summer sun beats down on the concrete, and heat shimmers over the rooftops below.",
 .autumn = "A cool autumn wind blows across the roof, and the sky looks high and clear.",
 .winter = "A cold winter wind cuts across the roof, and the sea below looks grey."},
// ---- Iwatodai Dorm (F102) ----
{.major = 102, .minor = 101, .outdoor = true, .town = true,
 .base = "Dorm Entrance: a quiet street in Iwatodai. The dormitory is a handsome old building of dark red brick with "
         "tall arched windows patterned in art deco ironwork. A short flight of stone steps rises under a pitched "
         "portico to the double front doors, lit by a single lamp, and an iron fence runs along the front. Across the "
         "street a guardrail lines the sidewalk; utility poles carry wires overhead, office buildings stand nearby, and "
         "now and then a car or someone walking a dog goes by.",
 .day = "The street is calm and sunlit.",
 .evening = "The streetlights flicker on as the sunset fades behind the dorm.",
 .night = "The dorm's tall windows glow warm yellow against the dark, quiet street.",
 .spring = "Fresh green leaves fill the trees along the street.",
 .summer = "The trees are in deep summer green, and the air is heavy and hot.",
 .autumn = "The roadside trees have turned, and fallen leaves gather along the curb and the steps.",
 .winter = "The trees are bare, and dry leaves collect in drifts by the steps and along the curb."},
{.major = 102, .minor = 102, .floor = 1,
 .base = "The dorm lounge, on the first floor. The dorm feels like an old Western hotel. Just inside the front doors is "
         "a curved wooden reception counter with the dorm register you sign to save, and a little bell. The floor is "
         "carpeted in a green and cream check, the walls are panelled in wood beneath cream wallpaper, and brass lamps "
         "glow on the walls. In the lounge, red velvet sofas and armchairs with carved wooden frames sit around a low "
         "table, near an old-fashioned TV in a wooden cabinet. Bookshelves, a bulletin board, a computer desk and a post "
         "box stand about. Behind glass partitions with brass posts is the dining room under a chandelier, with the "
         "kitchen and its fridge beyond; a storeroom and the rear entrance are off to the side, and a red-carpeted "
         "staircase with a wooden banister climbs to the rooms. Your dormmates often relax here in the evenings.",
 .dark = "It is the Dark Hour. The lamps are dead and the lounge is washed in a sickly green glow; the clock has "
         "stopped, and the room feels hollow and still.",
 .christmas = "A tall Christmas tree stands in the lounge, hung with ornaments and twinkling lights."},
{.major = 102, .minor = 102, .floor = 2,
 .base = "The dorm's second floor, the boys' floor. The stairs come up beside a wooden banister onto a red carpet "
         "runner. A sitting area opens here: red upholstered benches around a low table on a rug, a glass-fronted "
         "bookcase, potted plants, a floor lamp, a candle-style chandelier, and tall windows with roll-down blinds. A "
         "vending machine stands in the corner by a bin. Along the hallway are your room, Sanada's room and vacant rooms "
         "waiting for new residents. Stairs lead down to the lounge and up to the third floor.",
 .dark = "It is the Dark Hour. The hallway is dead quiet under the green light, and the vending machine's glow has gone out."},
{.major = 102, .minor = 102, .floor = 3,
 .base = "The dorm's third floor, the girls' floor. The stairs arrive by a wooden banister onto a red carpet runner. The "
         "open area here is a little dining nook: two wooden tables with spindle-backed chairs, a pair of blue drink "
         "vending machines against the wall, a floor lamp, a potted plant, a candle-style chandelier and tall windows "
         "with roll-down blinds. Mitsuru's and Yukari's rooms and vacant rooms line the hallway. Stairs lead "
         "down to the second floor and up to the fourth.",
 .dark = "It is the Dark Hour. The hallway is silent and bathed in green light."},
{.major = 102, .minor = 102, .floor = 4,
 .base = "The dorm's fourth floor. A quieter landing with a pair of red benches, a floor lamp and a potted plant. The "
         "hallway leads to a large, heavy door: the command room, where the members of SEES meet. There are restrooms "
         "here, and stairs lead down to the rooms and up to the rooftop landing.",
 .dark = "It is the Dark Hour. The hallway is bathed in green light; behind the large door, the command room is "
         "where the team gathers on nights like this."},
{.major = 102, .minor = 102, .floor = 5,
 .base = "The rooftop landing at the top of the dorm stairs: a cramped, dim stairwell used for storage, with stools "
         "stacked upside down on an old table and a couple of spare floor lamps. The door out to the roof is under a "
         "green emergency exit sign, with a notice taped to it."},
{.major = 102, .minor = 121,
 .base = "Your Room, on the dorm's second floor. A small single room with wooden floorboards and pale green striped "
         "wallpaper. By the door are a tall white built-in closet, with moving boxes still stacked on the shelves above "
         "it, a small rounded old fridge, and a sink with a mirror, a towel rail and shelves of folded towels. Further "
         "in are your bed by the window with a radiator beneath it, a desk with a computer, a TV and a calendar on the "
         "wall. A shelf holds the things you have collected. It is plain, but it is yours.",
 .day = "Daylight comes in through the window over the bed.",
 .evening = "The setting sun throws orange light across the room through the window.",
 .night = "Outside the window it is night; the room is lit by its lamp.",
 .dark = "It is the Dark Hour. The lamp is dead, and the window spills a sickly green light across the bed and the "
         "floor under a huge, pale moon. Everything is utterly silent."},
{.major = 102, .minor = 125,
 .base = "Aragaki's Room. Shinjiro's room on the dorm's second floor, laid out like the other rooms: sparse and "
         "lived-in, with little more than his bed, his desk and a few belongings, kept the way he left them."},
{.major = 102, .minor = 141,
 .base = "The command room on the dorm's fourth floor, the meeting room of SEES. Green-striped wallpaper above wood "
         "panelling, white columns, and tall arched windows with iron grilles. One wall is taken up by a big console "
         "deck: a wide screen of glowing graphs and waveforms flanked by huge loudspeakers, with a desk of smaller "
         "monitors and a chair before it. Across the room a red armchair and cushioned stools sit around a long wooden "
         "table on a hide rug, beside tall bookshelves, a decorative display shelf and a storage room.",
 .dark = "It is the Dark Hour. The whole room is drowned in green; the arched windows blaze with sickly yellow-green "
         "moonlight and throw the pattern of their iron grilles across the floor."},
{.major = 102, .minor = 151, .outdoor = true,
 .base = "The dorm rooftop. A flat, tiled roof behind a low parapet. The brick stairwell housing has a plain metal door "
         "under a single lamp; big pipes and machinery units sit on one side, and on the other a rooftop garden of "
         "planter boxes, with sacks of soil and gardening tools beside them and a small storage shed. From up here you "
         "can see across the rooftops of Iwatodai.",
 .day = "The garden beds bask in the open sun.",
 .evening = "The sunset paints the sky and the garden in orange.",
 .night = "The neighbourhood's lights spread out below a wide night sky.",
 .dark = "It is the Dark Hour. The sky is a sickly green, and an enormous yellow moon hangs low and close over the "
         "silent town; not a single light burns anywhere below.",
 .spring = "Mild spring air; the beds are ready for planting.",
 .summer = "The summer air is warm even up here.",
 .autumn = "A cool autumn wind blows across the roof.",
 .winter = "The winter wind is cold and sharp up here."},
// ---- Paulownia Mall (F103) ----
{.major = 103, .minor = 101, .outdoor = true, .town = true,
 .base = "Paulownia Mall, Fountain Square: a round plaza under a high glass dome, ringed by elegant European-style "
         "shopfronts with arched upper windows, columns and wall lanterns. The paving is laid in wide concentric rings "
         "around a tiered fountain at the centre, with benches and potted trees around it. Behind the fountain, stairs "
         "climb to the karaoke bar Mandragora. Around the ring are Chagall Café with its round sign, the accessory shop "
         "Be Blue V, Mayoido Antiques, Aohige Pharmacy, the CD and DVD shop Kenneth Arnold, the police station, and the "
         "arcade Game Parade with crane games outside, besides a bulletin board and the neon-lit entrance of Club "
         "Escapade. Tucked away in a corner is a strange blue door that only you seem to notice: the way into the "
         "Velvet Room.",
 .day = "Daylight falls through the glass dome, the plaza bustles with shoppers, and the fountain sparkles.",
 .evening = "Shop signs light up as the dome turns orange overhead, and students crowd the plaza after school.",
 .night = "The dome shows a starry night sky. The fountain glows blue, neon signs blaze over Escapade, Mandragora and "
          "Game Parade, and Be Blue V and the antique shop have pulled down their shutters; the crowd is older now.",
 .dark = "It is the Dark Hour. The neon is dead, the fountain hangs silent, and coffins stand where shoppers were; "
         "dark puddles like blood stain the paving under the green dome.",
 .spring = "", .summer = "The summer heat hangs over the plaza, and people linger near the cool spray of the fountain.",
 .autumn = "", .winter = "Shoppers hurry through the plaza bundled in winter coats.",
 .christmas = "For Christmas the plaza is decorated with Christmas trees strung with glowing lights, and a large "
              "illuminated tree stands at its heart."},
{.major = 103, .minor = 111,
 .base = "Club Escapade, a nightclub in Paulownia Mall. A dark room pulsing with music: a crowd dances on a floor where "
         "the club's name is projected in red spotlights, laser beams sweep overhead, and strips of light glow along "
         "the edges. At one end is a stage with a drum kit, microphone stands and towering speaker stacks; around the "
         "floor are lounge tables lit by small glowing lamps, and a bar where bottles and brandy glasses glitter.",
 .day = "", .evening = "", .night = "",
 .dark = "It is the Dark Hour. The music has stopped dead; the coloured lights are frozen and dark, and the room is "
         "filled with silent coffins."},
// ---- Port Island Station (F104) ----
{.major = 104, .minor = 101, .outdoor = true, .town = true,
 .base = "Port Island Station: a wide paved plaza inlaid with blue lines, dotted with tall classical columns and "
         "double-headed lampposts. Raised flowerbeds full of tall purple flowers have benches set into them, and "
         "sparrows hop about the paving. Broad stone steps climb to the brick station entrance and its signboard, with "
         "the ticket booth and the restrooms up top. The cinema Screen Shot faces the plaza under big movie posters, "
         "beside a café and a vending machine, and a department store, Tatsumi Mall, hangs its sale banners on one side.",
 .day = "Commuters and students stream across the bright plaza.",
 .evening = "The evening sun slants across the plaza, the columns throw long shadows, and the cinema's sign lights up.",
 .night = "The plaza is lit by the lampposts, the cinema's glow and the station lights; late commuters hurry past.",
 .spring = "The flowerbeds are in bloom.", .summer = "The flowerbeds are lush under the summer sun.",
 .autumn = "", .winter = "The flowerbeds are bare and the plaza is cold."},
{.major = 104, .minor = 102, .outdoor = true, .town = true,
 .base = "Station Outskirts: the back alley behind Port Island Station. A cramped, grimy space hemmed in by red brick "
         "walls studded with air conditioner units and pipes. Oil drums and old boxes are piled by a stairway, and black "
         "garbage bags and scattered magazines spill around a big dumpster. The Mahjong Parlor Red Hawk and the bar Que "
         "Sera Sera have their doors here. It is a hangout for delinquents, and most students know to stay away. A "
         "familiar-looking black cat sometimes prowls about.",
 .day = "Even in daylight the alley feels shadowed and stale.",
 .evening = "The alley falls into shadow early as the sun sets.",
 .night = "The alley is dark, lit only by a few buzzing lights and the signs over the bar and the mahjong parlor.",
 .spring = "", .summer = "The summer heat makes the alley stink of garbage.", .autumn = "", .winter = ""},
// ---- Iwatodai Station and Strip Mall (F105) ----
{.major = 105, .minor = 101, .outdoor = true, .town = true,
 .base = "Iwatodai Station, the station closest to the dorm. A concrete concourse under fluorescent lights: posters "
         "and a station map on the walls, a row of recycling bins, a long bench, and crows pecking at the floor. Overhead "
         "a departure board lists the trains, and a wide staircase flanked by escalators climbs to the platforms, its "
         "steps plastered with banners advertising a shopping mall. There is also an elevator, a telephone box, a "
         "vending machine, bicycle racks out front, and a construction site fenced off nearby. The strip mall is right "
         "next door.",
 .day = "Commuters come and go from the escalators in the daylight.",
 .evening = "The station grows busy with people coming home as the sky turns orange.",
 .night = "The station's fluorescent lights shine bright against the night; only a few travellers are about.",
 .dark = "It is the Dark Hour. The escalators have stopped, the lights are dead, and coffins stand on the concourse "
         "under a green sky.",
 .spring = "", .summer = "", .autumn = "", .winter = "It is winter, and the air at the station is cold."},
{.major = 105, .minor = 102, .floor = 1, .outdoor = true, .town = true,
 .base = "Iwatodai Strip Mall, ground floor. A three-storey brick building whose shops open onto stacked balconies "
         "and walkways, joined by stairs and an iron spiral staircase. At ground level, Wilduck Burger shines under its "
         "big orange sign, beside the used bookstore Bookworms. Bicycles are parked along the front, and people sit on "
         "the kerb with their shopping.",
 .night = "The burger shop's sign and the shop lights glow warmly in the dark.",
 .spring = "", .summer = "", .autumn = "", .winter = ""},
{.major = 105, .minor = 102, .floor = 2, .outdoor = true, .town = true,
 .base = "Iwatodai Strip Mall, second floor. An open walkway of stone tiles behind a metal railing, beside the iron "
         "spiral staircase. The Chinese diner Wakatsu Kitchen has a green sign and a display case of plastic food "
         "samples; Hagakure Ramen hangs big blue cloth banners with a flower crest at its door, and people queue outside. "
         "The sweets shop Azuki Arai, with its mascot Azuki-chan, is here too.",
 .night = "The restaurants' lit signs and warm windows glow along the walkway.",
 .spring = "", .summer = "", .autumn = "", .winter = ""},
{.major = 105, .minor = 102, .floor = 3, .outdoor = true, .town = true,
 .base = "Iwatodai Strip Mall, third floor. The top walkway, reached by the iron spiral staircase. The MangaStar net café "
         "has a purple sign scattered with stars, a tall banner flag out front, and windows plastered with a cartoon "
         "mascot girl with antennae and posters for new games. Umiushi Beef Bowls is up here too, beside a drink "
         "vending machine, and you can look down over the railing at Hagakure's blue banners below.",
 .night = "The net café's sign glows; it stays open late.",
 .spring = "", .summer = "", .autumn = "", .winter = ""},
// ---- Naganaki Shrine (F106) ----
{.major = 106, .minor = 101, .outdoor = true, .town = true,
 .base = "Naganaki Shrine: a small Shinto shrine among old trees in Iwatodai. A mossy stone path leads to the wooden "
         "main hall, roofed in green copper tiles, where a thick braided straw rope with white paper streamers hangs over "
         "the entrance above the offertory box. Stone guardian lion-dogs flank the steps. To one side a tunnel of small "
         "red torii gates leads to the Inari fox statues; a roofed stone basin is there for cleansing your hands, a huge "
         "sacred tree spreads its branches overhead, and fortune slips are tied in rows on a rack. Beside the grounds is "
         "a small playground: a slide, a jungle gym, a see-saw, a horizontal bar and a sandbox.",
 .day = "It is peaceful here; birdsong and the rustle of leaves replace the noise of the town.",
 .evening = "The low sun filters through the trees in long beams, and the playground stands empty and quiet.",
 .night = "The shrine is dark and hushed, the trees black against the sky.",
 .spring = "The trees around the grounds are fresh with new green.",
 .summer = "The trees are dense and green, and the shade is welcome in the heat.",
 .autumn = "The trees have turned red and gold, and fallen leaves cover the paving and the playground.",
 .winter = "The trees are bare, and the shrine grounds look stark and cold."},
// ---- Yakushima (F107) ----
{.major = 107, .minor = 101, .outdoor = true,
 .base = "The Kirijo family's private beach on Yakushima. White sand slopes down to clear turquoise water, fringed with "
         "palm and banana trees. Striped beach parasols shade wooden loungers, driftwood lies on the shore, a boat is "
         "tied up at a wooden pier, and a dark rocky headland closes off the end of the beach. Behind it, the Kirijo Yakushima Mansion stands at the edge of a deep, green "
         "forest of ancient cedars.",
 .day = "The summer sun blazes on the sand, and the sea glitters.",
 .evening = "The sun sinks into the sea, turning the water and the sand orange.",
 .night = "The beach is dark; waves wash softly on the sand under the stars.",
 .spring = "", .summer = "", .autumn = "", .winter = ""},
// ---- Kyoto, the school trip (F108) ----
{.major = 108, .minor = 101, .floor = 1,
 .base = "Godaigo Inn, a traditional Japanese inn in Kyoto, on its ground floor. The lobby is carpeted in wine red with a "
         "small flower pattern under a slatted wooden ceiling. Guests in cotton robes sit on benches draped in red cloth "
         "at low wooden tables, between standing paper lanterns, beside a round red-lacquered display shelf of tea "
         "things under a red paper parasol, and a gold folding screen. Paper-paned windows look onto a lit inner garden "
         "with a stone lantern, hanging scrolls line the wall, and there is a pamphlet rack and a souvenir corner piled "
         "with Kyoto sweets, crafts, decorated figurines and wooden swords. A passage leads to the hot spring."},
{.major = 108, .minor = 101, .floor = 2,
 .base = "Godaigo Inn, second floor. A quiet wooden corridor of guest rooms behind sliding doors, among them the Fuji "
         "Room, with a vending machine, a telephone and the emergency exit."},
{.major = 108, .minor = 101, .floor = 3,
 .base = "Godaigo Inn, third floor. Another wooden corridor of guest rooms, with a vending machine, a telephone, a "
         "travel guide and the emergency exit."},
{.major = 108, .minor = 102, .name = "Hot Spring", .outdoor = true,
 .base = "The inn's open-air hot spring. A wide pool of steaming water is edged with great smooth boulders, with a "
         "little cascade spilling between them. A bamboo fence and maple trees screen it in, and along the bathhouse "
         "wall is a row of washing taps with low stools and buckets; the air is warm and damp.",
 .night = "Steam rises into a sky full of stars, and small lamps glow along the bathhouse wall.",
 .spring = "", .summer = "", .autumn = "The maples around the pool are red.", .winter = ""},
{.major = 108, .minor = 104, .base = "The Nanten Room, a guest room at Godaigo Inn named for the nandina shrub. A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 105, .base = "The Botan Room, a guest room at Godaigo Inn named for the peony. A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 106, .base = "The Tsubaki Room, a guest room at Godaigo Inn named for the camellia. A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 107, .base = "The Shoubu Room, a guest room at Godaigo Inn named for the iris. A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 108, .base = "The Kikyo Room, a guest room at Godaigo Inn named for the bellflower. A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 109, .base = "The Ajisai Room, a guest room at Godaigo Inn named for the hydrangea. A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 110, .base = "The Suisen Room, a guest room at Godaigo Inn named for the daffodil. A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 111, .base = "The Sazanka Room, a guest room at Godaigo Inn named for the sasanqua camellia. "
 "A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
{.major = 108, .minor = 112, .base = "The Suzuran Room, a guest room at Godaigo Inn named for the lily of the valley. "
 "A "
 "traditional room of tatami mats, with futons laid out for the night, a low red-lacquered table set with a tea set and legless chairs on floor cushions, a square paper lamp hanging from the wooden ceiling, painted sliding closet doors, a small old TV, a round window and a paper-paned sliding door."},
// ---- The full-moon operations and the first night (Dark Hour only) ----
{.major = 302, .minor = 102, .name = "Inside Monorail",
 .base = "Inside the monorail, stalled on its elevated track in the Dark Hour. A narrow car of bench seats along both "
         "walls, hand straps hanging still from the rails overhead, advertisements dangling from the ceiling, and "
         "connecting doors leading car after car toward the front of the train. Upright coffins stand where the "
         "passengers were, oozing a red glow onto the floor. Through the windows lie the sea and the silent city under "
         "a green sky and a huge moon.", .dark_only = true},
{.major = 302, .minor = 101, .name = "Overhead Bridge",
 .base = "The Overhead Bridge: the monorail's elevated concrete track, high above the city in the Dark Hour. The "
         "guideway curves away between low walls, with steel gantries spanning it overhead and office buildings close "
         "alongside; a red splash stains the track. Far ahead, the stalled train waits. Everything is lit a sickly "
         "green under an enormous moon.", .dark_only = true},
{.major = 304, .minor = 101, .name = "Hotel Hallway",
 .base = "Champs de Fleurs, a love hotel on Shirakawa Boulevard, transformed by the Dark Hour. The hallways are carpeted "
         "in a dull mauve check, the walls papered with faded flowers, with potted palms and green-shaded lamps along "
         "them. On the ground floor are the entrance, a reception window, an information panel of room photos, "
         "curtained waiting booths, vending machines and elevators. Above, corridors of numbered doors run across three "
         "floors, with a strange vending machine on each landing. Coffins stand where guests were caught, and everything "
         "is lit a sickly green.", .dark_only = true},
{.major = 304, .minor = 104, .name = "Hierophant's Chamber",
 .base = "A room in the love hotel where the Shadow lies in wait. Mirrors cover the walls, and the room's garish "
         "furnishings are drowned in green Dark Hour light.", .dark_only = true},
{.major = 304, .minor = -1, .name = "Hotel Room",
 .base = "A love hotel room on Shirakawa Boulevard: a big round bed with a red cover, a large mirror framed in heart "
         "ornaments, little round mirrors along the flowered wallpaper, a sofa and an oval glass coffee table, potted "
         "palms, a heart-shaped chair, a glass-walled bathroom, a TV and a small fridge. In the Dark Hour it is all "
         "washed in sickly green light.",
 .dark_only = true},
{.major = 305, .minor = 101, .name = "Underground",
 .base = "An abandoned military base deep underground, entered through heavy riveted steel doors streaked with rust. Cracked "
         "concrete slabs pave the floors, old timber beams shore up the walls, and cobwebs hang in the corners. "
         "Passages descend "
         "level by level, with signs marking 13 and then 17 meters underground. Rusting old military equipment, cannon "
         "and artillery parts lie about, and side rooms hold beds, boxes, canned food, desks with diaries left behind, and "
         "a messy work area. The air is stale and still.", .dark_only = true},
{.major = 308, .minor = 101, .name = "Moonlight Bridge",
 .base = "The Moonlight Bridge, the huge suspension bridge linking Port Island to the mainland, in the Dark Hour. Its "
         "towers and cables soar into a green, misty sky beneath an enormous moon. Cars and trucks sit abandoned in their "
         "lanes, coffins stand where people were, streaks like blood smear the asphalt, the city skyline rises beyond, "
         "and the dark sea stretches far below.", .dark_only = true},
{.major = 150, .minor = 211,
 .base = "A vast open floor of polished blue marble slabs, high in a tower, under rings of glowing arched windows and "
         "great turning gears. The air is still and empty.", .dark_only = true},
{.major = 150, .minor = 109, .name = "Dark Hour Cityscape",
 .base = "The city in the Dark Hour, swallowed by a thick, featureless green haze: no street or building can be made "
         "out through it, and not a sound carries.", .dark_only = true},
{.name = "Iwatodai Station",
 .base = "Iwatodai Station at midnight, in the Dark Hour. The station's lights have died all at once; the escalators "
         "stand frozen, and the concourse is empty but for upright coffins where people were. A sickly green glow lies "
         "over everything beneath a huge, swollen moon.", .dark_only = true},
{.name = "School Dorm",
 .base = "The dormitory at the end of the street, its arched windows dark in the Dark Hour. Green moonlight falls over "
         "its steps and heavy front doors.", .dark_only = true},
// ---- Tartarus (its blocks' special floors are fields 202-207) ----
{.major = 201, .minor = 101, .name = "Tartarus: Gates",
 .base = "Tartarus: Gates. This is Gekkoukan High's front approach, but in the Dark Hour the school has twisted and "
         "risen into Tartarus: a colossal tower of contorted, mismatched shapes stacked impossibly high, spiralling up "
         "into the green sky toward the enormous moon. The trees along the old approach still stand at its foot.",
 .spring = "Strangely, the cherry trees by the gate are still in bloom.",
 .autumn = "The trees by the gate still wear their autumn colours, eerie in the green light.", .dark_only = true},
{.major = 201, .minor = 102, .name = "Tartarus: Entrance",
 .base = "Tartarus: Entrance. A vast, palatial hall. A blue carpet runs down the middle of a polished floor of blue and "
         "white marble laid in a maze-like pattern and circled by rings of gold, between tall white columns; the walls "
         "behind glow green, lined with arched alcoves. Ahead, a long blue staircase climbs to a gigantic gilded clock "
         "face, its several dials and turning gears set in ornate golden scrollwork: the way up into the tower. To one "
         "side stands the blue door of the Velvet Room, nearby a grandfather clock and Mitsuru's motorcycle, and on the "
         "other side a small arched device that carries you to floors you have reached.", .dark_only = true},
{.name = "Monad Passage",
 .base = "The Monad Passage, a hidden way beneath Tartarus's entrance hall: dark corridors that feel even heavier "
         "and more hostile than the tower above.", .dark_only = true},
{.name = "Monad Chambers",
 .base = "The Monad Chambers, a hidden area beyond a secret gate in the Abyss of Time, floating high above the floor you came from. It is built like the labyrinth below, but harsher: the same halls are lit by glowing red lamps and thin red strips of light, in a hot, dusty haze, and the strongest Shadows wait here. The exit stands near where you arrive.", .dark_only = true},
{.name = "Monad",
 .base = "Monad, a hidden area beyond a Monad door, floating high above the floor you came from. It is built like the block below, but drenched in harsh red and orange light, with dark puddles like blood smeared across the floors and red glowing arches in a pale haze. The exit door stands near where you arrive.", .dark_only = true},
{.major = 202, .minor = -1, .name = "Thebel",
 .base = "Tartarus, Thebel block, the first block of the tower: a maze that reshapes itself every night. Corridors of dark "
         "blue stone, their walls stepping back in tiers like a ziggurat, run over checkered tile floors edged with "
         "glowing green diamond patterns, and pale green light glimmers in the gloom. Here and there huge gears are "
         "built into the architecture, as if the tower were a vast piece of clockwork.", .dark_only = true},
{.major = 203, .minor = -1, .name = "Arqa",
 .base = "Tartarus, Arqa block, the second block: a maze that feels alive. Its walls are great twisting, ribbed forms "
         "like tendrils or giant coiled arms, knotted overhead, their ridges glowing a vivid magenta and violet against "
         "the green gloom; the floor underfoot is uneven and rippled.", .dark_only = true},
{.major = 204, .minor = -1, .name = "Yabbashah",
 .base = "Tartarus, Yabbashah block, the third block: an industrial maze of steel girders and lattice towers, huge "
         "gears set into the walls, and long walkways leading to arches crowned with glowing stained-glass windows, "
         "lit in deep red and gold, with dark stains like blood on the floors.", .dark_only = true},
{.major = 205, .minor = -1, .name = "Tziah",
 .base = "Tartarus, Tziah block, the fourth block: a dark gothic palace. A red carpet runs down long halls between "
         "rows of spiked iron railings, past massive carved columns and great stone urns, over floors inlaid with "
         "circular medallions, toward tall arched stained-glass windows, all in a pale, washed-out golden light.",
 .dark_only = true},
{.major = 206, .minor = -1, .name = "Harabah",
 .base = "Tartarus, Harabah block, the fifth block: a psychedelic maze like a nightclub gone mad. The floors are "
         "glowing checkerboards of magenta, cyan, red and blue; the walls are forests of dark pillars topped with neon "
         "blocks in every colour, swirling with concentric dotted patterns, with striped columns and gears looming in "
         "the dark, and dark puddles like blood here and there.", .dark_only = true},
{.major = 207, .minor = 104,
 .base = "A floor of Tartarus built like its first block: corridors of dark stone stepping back in tiers, a checkered "
         "floor edged with glowing green diamond patterns, a circle of glowing green lights on the floor, and a tall "
         "arched stained-glass window glowing green at the end of the hall.", .dark_only = true},
{.major = 207, .minor = -1, .name = "Adamah",
 .base = "Tartarus, Adamah block, the sixth and last block, near the top of the tower. The floors are dizzying "
         "op-art: bold black and white stripes that sweep in curves along winding paths. All around rise towering white "
         "and grey monoliths carved with zigzag patterns, like a city of pale towers, under a dark green sky, with "
         "touches of red light here and there.", .dark_only = true},
{.name = "Tartarus",
 .base = "Tartarus, the tower of the Dark Hour. A maze of corridors and rooms that reshapes itself every night, lit "
         "an eerie green, where Shadows roam.", .dark_only = true},
{.major = 201, .minor = 103,
 .base = "A room all in deep velvet blue: walls of ornate patterned panels, a tufted blue wall, a tufted blue sofa "
         "and a round table draped in blue cloth, in a dim blue light.", .dark_only = true},
{.major = 401, .minor = 103,
 .base = "A room all in deep velvet blue: walls of ornate patterned panels, a tufted blue wall, a tufted blue sofa "
         "and a round table draped in blue cloth, in a dim blue light.", .dark_only = true},
// ---- Episode Aigis: the Abyss of Time ----
{.major = 401, .minor = 102, .name = "Desert of Doors",
 .base = "The Desert of Doors, at the bottom of the Abyss of Time. An endless expanse of pale golden sand, rippled in "
         "wide rings, under a hazy white sky. Tall cream-coloured doors trimmed with gold ornament stand alone in the "
         "sand around you, each one leading into a labyrinth born of someone's past, with the blue door of the Velvet "
         "Room among them and a glowing point of light that carries you onward. Time does not move here.",
 .dark_only = true},
{.major = 402, .minor = -1,
 .base = "The landing at the gate of a labyrinth in the Abyss of Time: a raised platform of polished black and gold "
         "tiles inlaid with swirling gold patterns, floating in a bright golden haze over the sand. At its head stands "
         "a tall arched double door of gold, richly carved, between two burning torches on iron stands.",
 .dark_only = true},
{.major = 403, .minor = -1,
 .base = "The landing at the gate of a labyrinth in the Abyss of Time: a raised platform of polished black and gold "
         "tiles inlaid with swirling gold patterns, floating in a bright golden haze over the sand. At its head stands "
         "a tall arched double door of gold, richly carved, between two burning torches on iron stands.",
 .dark_only = true},
{.major = 404, .minor = -1,
 .base = "The landing at the gate of a labyrinth in the Abyss of Time: a raised platform of polished black and gold "
         "tiles inlaid with swirling gold patterns, floating in a bright golden haze over the sand. At its head stands "
         "a tall arched double door of gold, richly carved, between two burning torches on iron stands.",
 .dark_only = true},
{.major = 405, .minor = -1,
 .base = "The landing at the gate of a labyrinth in the Abyss of Time: a raised platform of polished black and gold "
         "tiles inlaid with swirling gold patterns, floating in a bright golden haze over the sand. At its head stands "
         "a tall arched double door of gold, richly carved, between two burning torches on iron stands.",
 .dark_only = true},
{.major = 406, .minor = -1,
 .base = "The landing at the gate of a labyrinth in the Abyss of Time: a raised platform of polished black and gold "
         "tiles inlaid with swirling gold patterns, floating in a bright golden haze over the sand. At its head stands "
         "a tall arched double door of gold, richly carved, between two burning torches on iron stands.",
 .dark_only = true},
{.major = 407, .minor = -1,
 .base = "The landing at the gate of a labyrinth in the Abyss of Time: a raised platform of polished black and gold "
         "tiles inlaid with swirling gold patterns, floating in a bright golden haze over the sand. At its head stands "
         "a tall arched double door of gold, richly carved, between two burning torches on iron stands.",
 .dark_only = true},
{.major = 408, .minor = -1,
 .base = "The landing at the gate of a labyrinth in the Abyss of Time: a raised platform of polished black and gold "
         "tiles inlaid with swirling gold patterns, floating in a bright golden haze over the sand. At its head stands "
         "a tall arched double door of gold, richly carved, between two burning torches on iron stands.",
 .dark_only = true},
{.major = 409, .minor = -1,
 .base = "The approach to Colosseo Purgatorio, an arena deep in the Abyss of Time: a round terrace of polished marble "
         "with a red carpet edged in gold running across it, wrought-iron candelabras with lit candles, the massive "
         "bronze bases of great columns, and a freestanding ornate door. Beyond the rim, a field of golden flowers "
         "stretches into a glowing haze.", .dark_only = true},
{.major = 170, .minor = 211, .name = "Colosseo Purgatorio",
 .base = "Colosseo Purgatorio, the arena of the Abyss of Time: a wide floor of raked sand ringed by towering walls "
         "with balconies high above, all washed in a hazy golden light.", .dark_only = true},
{.name = "Malebolge",
 .base = "Malebolge, the first labyrinth behind the doors of the Abyss of Time: a stately old hall like the corridor of a grand public building: thick round columns banded in bronze, globe lamps on the walls, high windows of red glass, and a long green runner over a worn tile floor, all in a dim, dusty amber light.", .dark_only = true},
{.name = "Cocytus",
 .base = "Cocytus, a labyrinth behind the doors of the Abyss of Time: a nightmarish violet dungeon. Webs and glistening purple slime cover the floor and stone walls, gothic iron fences and a great barred gate close off the way, clusters of round lights glow overhead, and a huge grinning face leers down from behind the bars.", .dark_only = true},
{.name = "Caina",
 .base = "Caina, a labyrinth behind the doors of the Abyss of Time: a cold high-tech maze: steel walkways lined with thick pipes run between walls of glowing blue hexagonal grids, with security cameras in round frames watching from the walls and glowing blue rings and hanging screens of symbols overhead.", .dark_only = true},
{.name = "Antenora",
 .base = "Antenora, a labyrinth behind the doors of the Abyss of Time: an ancient temple of carved sandstone. Square pillars with round glowing lamps line a paved processional way, screens of geometric lattice stand between them, and pale green light spills from a doorway far ahead.", .dark_only = true},
{.name = "Ptolomea",
 .base = "Ptolomea, a labyrinth behind the doors of the Abyss of Time. Its first floors are a stately old hall like the corridor of a grand public building: thick round columns banded in bronze, globe lamps on the walls, high windows of red glass, and a long green runner over a worn tile floor, all in a dim, dusty amber light.", .dark_only = true},
{.name = "Judecca",
 .base = "Judecca, a labyrinth behind the doors of the Abyss of Time: a cold high-tech maze: steel walkways lined with thick pipes run between walls of glowing blue hexagonal grids, with security cameras in round frames watching from the walls and glowing blue rings and hanging screens of symbols overhead.", .dark_only = true},
{.name = "Empyrean",
 .base = "Empyrean, the last labyrinth behind the doors of the Abyss of Time: a golden cathedral. Huge twisted marble columns rise from a polished checkerboard floor of gold and black inlaid with circular medallions, red marble walls and hanging banners lead the eye to a tall stained-glass window glowing at the end of the nave, and warm golden light pours over everything.", .dark_only = true},
};
