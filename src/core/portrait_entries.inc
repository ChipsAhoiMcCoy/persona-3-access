// The dialogue portraits (included by portrait_text.cpp): every outfit of every character the game has a
// bust-up for (T_BU_<PC|SC|NC><char>_Pose<x>_C<outfit>, 155 outfits in 264 poses; three placeholder textures with no table row are never shown), each rendered from the game's own
// textures and looked at (tools/bustup/outfits.py, 2026-11-12). An outfit's text is the character's look in it; a
// pose's text is said after it. Character ids: 1 protagonist, 2 Yukari, 3 Junpei, 4 Akihiko, 5 Mitsuru, 6 Fuuka,
// 7 Aigis, 8 Ken, 9 Koromaru, 10 Shinjiro, 11 Metis, 12 Aigis (Episode Aigis); 1xx Social Links, 2xx story, 1xxx others.
const Outfit kOutfits[] = {
  {1, 0, "The protagonist, a slim teenage boy with dark blue hair cut in a bob, its long fringe falling over "
         "his right eye, and grey-blue eyes. He wears a plain dark grey top with a small upright collar."},
  {2, 1, "Yukari, a slim teenage girl with short, chin-length light brown hair that flips outward at the ends "
         "and brown eyes, wearing small silver stud earrings. She wears a pale grey choker with a heart-shaped "
         "buckle and a white short-sleeved button-up blouse, collar open, with a breast pocket: her school "
         "summer uniform."},
  {2, 2, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
         "silver stud earrings and a pale grey choker with a heart-shaped buckle. She wears her school "
         "uniform: a pink cardigan over a white shirt with a wide pointed collar and a large red bow at the "
         "throat."},
  {2, 5, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
         "silver stud earrings and a pale grey choker with a heart-shaped buckle. She wears casual summer "
         "clothes: a pink camisole with thin red-edged straps and a row of red dots down the front, leaving "
         "her shoulders bare."},
  {2, 6, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
         "silver stud earrings and a pale grey choker with a heart-shaped buckle. She wears casual winter "
         "clothes: an off-the-shoulder pink ribbed knit top with a thick white fur trim around the neckline, "
         "with thin dark straps over her bare shoulders."},
  {2, 51, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
          "silver stud earrings and a pale grey choker with a heart-shaped buckle. She wears her school "
          "uniform, a pink cardigan over a white shirt with a wide pointed collar and a large red bow, with a "
          "red armband on her left arm printed in black \"S.E.E.S.\" and \"Special Extracurricular Execution "
          "Squad\"."},
  {2, 52, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
          "silver stud earrings and a pale grey choker with a heart-shaped buckle. She is in combat gear: a "
          "dark grey and red zip-up jacket with a broad layered collar and a round black-and-white badge on the "
          "left sleeve, over a pink top, a white collared shirt and a large red bow. A quiver of black-shafted, "
          "white-fletched arrows sits over her right shoulder."},
  {2, 102, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings. She wears a swimsuit: a pink bikini top with teal halter straps tied behind "
           "her neck, and dark rectangular sunglasses pushed up on top of her head."},
  {2, 106, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings. She wears a maid outfit: a white frilled lace headdress on a black headband, "
           "and a black dress with a white bib and tall pointed white collar fastened with black buttons."},
  {2, 154, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings. Her shoulders and arms are bare above a pink towel wrapped around her chest, "
           "as at a hot spring bath."},
  {2, 155, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings. She wears an inn's robe: a white cotton yukata printed with grey linked rings, "
           "under a loose navy blue jacket."},
  {2, 156, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings. She wears a summer festival yukata in a pink and white checkerboard pattern, "
           "printed with yellow, pink and orange flowers on green stems, with a tan sash at the back."},
  {2, 157, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes. She wears a "
           "formal New Year's kimono in deep pink printed with large white cherry blossoms, a green-edged collar "
           "and a teal patterned sash, and a hair ornament of pink and white flowers with strands of dangling "
           "pink and silver petals over her left ear."},
  {2, 159, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings and a pale grey choker with a heart-shaped buckle. She wears a red long-sleeved "
           "top under a pink-and-white striped apron with dark red stitched straps and a front pocket."},
  {2, 201, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings and a pale grey choker with a heart-shaped buckle. She wears a dark grey and "
           "red zip-up jacket with a broad layered collar and a round black-and-white badge on the left sleeve, "
           "over a pink top, a white collared shirt and a large red bow: her combat gear without the quiver."},
  {3, 1, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
         "baseball cap forward, a white knotwork emblem on its front. This is the summer school uniform: a "
         "white short-sleeved shirt hanging open, its wide collar turned up, over a plain navy T-shirt. A "
         "silver chain necklace carries a small square tag set with red and green studs."},
  {3, 2, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
         "baseball cap with a white knotwork emblem. This is the winter school uniform: a charcoal-black "
         "blazer over a blue-grey collared shirt left unbuttoned at the neck, collar turned up, with a small "
         "green-and-yellow pin on the blazer's lapel. A silver chain necklace carries a small square tag set "
         "with red and green studs."},
  {3, 5, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
         "baseball cap with a white knotwork emblem. Casual summer clothes: a plain dark grey tank top that "
         "leaves his thin arms and shoulders bare. A silver chain necklace carries a small square tag set with "
         "red and green studs."},
  {3, 6, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
         "baseball cap with a white knotwork emblem. Casual winter clothes: a navy zip-up parka with a big "
         "shaggy brown fur collar, open over a plain white T-shirt. A silver chain necklace carries a small "
         "square tag set with red and green studs."},
  {3, 51, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
          "baseball cap with a white knotwork emblem. The winter school uniform, charcoal-black blazer over a "
          "blue-grey shirt open at the neck with a green-and-yellow lapel pin, as usual, but with a red "
          "S.E.E.S. armband around his upper arm. A silver chain necklace carries a small square tag set with "
          "red and green studs."},
  {3, 52, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
          "baseball cap with a white knotwork emblem. Combat gear: no blazer, just a blue-grey collared shirt "
          "open at the neck, a red S.E.E.S. armband on his upper arm, and a wide white leather strap with a "
          "steel buckle running diagonally across his chest. Strapped to his back is a big two-handed sword, "
          "its long black-gripped hilt and steel cross-guard rising over his shoulder. A silver chain necklace "
          "with a small studded square tag hangs at his throat."},
  {3, 102, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, is bare-chested, as at "
           "the beach or in the water. He wears his slate-blue baseball cap backwards, its adjustable strap and "
           "metal buckle over his forehead. His only other accessory is a silver chain necklace with a small "
           "square tag set with red and green studs."},
  {3, 154, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, is bare-chested, as in "
           "a bath or at the beach. He wears his slate-blue baseball cap backwards, its adjustable strap and "
           "metal buckle over his forehead. His only other accessory is a silver chain necklace with a small "
           "square tag set with red and green studs."},
  {3, 155, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
           "baseball cap with a white knotwork emblem. He is in an inn's robes: a white cotton yukata printed "
           "with grey rings, crossed loosely at the chest, under a dark indigo short jacket worn open. No "
           "necklace shows."},
  {3, 202, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, wears his slate-blue "
           "baseball cap with a white knotwork emblem. No blazer: a blue-grey collared shirt, open at the neck "
           "with the collar up, and a red S.E.E.S. armband on his upper arm. A silver chain necklace carries a "
           "small square tag set with red and green studs."},
  {3, 901, "Junpei, a lanky young man with short dark hair, grey eyes and a thin goatee, in the summer school "
           "uniform: his baseball cap with its knotwork emblem, a white short-sleeved shirt hanging open over a "
           "navy T-shirt, and the silver chain with its studded square tag. This version is drawn in dim, cold "
           "bluish light, the cap and shirt deep in shadow and dark shading under his eyes."},
  {4, 1, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
         "sticking plaster on the left side of his forehead. He wears the school summer uniform: a "
         "short-sleeved white shirt with a black ribbon tie, under a red knitted sweater vest with a small "
         "gold letter pin on the chest."},
  {4, 2, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
         "plaster on his forehead. He wears the school summer uniform: a white shirt with a black ribbon tie "
         "under a red knitted sweater vest with a small gold pin. A tan leather holster strap is buckled "
         "around his upper arm."},
  {4, 5, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
         "plaster on his forehead. He wears casual summer clothes: a deep red V-neck shirt with thin pale "
         "pinstripes, open at the throat."},
  {4, 6, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
         "plaster on his forehead. He wears casual winter clothes: a light grey coat with tan-edged lapels "
         "over a dark charcoal shirt, and a long red ribbed scarf wrapped around his neck with the ends "
         "hanging down his chest."},
  {4, 51, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
          "plaster on his forehead. He wears the school summer uniform, a white shirt with a black ribbon tie "
          "under a red knitted sweater vest, with a tan holster strap on his upper arm and a red armband around "
          "the sleeve below it."},
  {4, 52, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
          "plaster on his forehead. Over a white shirt, black ribbon tie and red knitted vest he wears an open "
          "jacket with dark grey front panels, large round brass snaps and bright red sleeves, each sleeve "
          "marked with a round black-and-white quartered patch."},
  {4, 102, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
           "plaster on his forehead. He wears a plain white crew-neck T-shirt printed on the chest with grey "
           "lettering reading \"SCUBA DIVING\" above a large \"SOS\"."},
  {4, 154, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
           "plaster on his forehead. He is shirtless, showing bare, muscular shoulders and chest, as at a bath "
           "or hot spring."},
  {4, 155, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
           "plaster on his forehead. He wears an inn's yukata: a white robe printed with a grey chain-of-circles "
           "pattern, under a loose slate-blue jacket worn open over it."},
  {4, 159, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
           "plaster on his forehead. He wears a white collarless shirt with faint pinstripes, open at the neck, "
           "under a dark red bib apron with a stitched front pocket."},
  {5, 1, "Mitsuru, a tall young woman with long, wavy crimson hair falling past her shoulders, a long fringe "
         "sweeping over her left eye, and red eyes. She wears the school uniform: a white high-collared blouse "
         "with full, puffed sleeves and a large red bow tied at the throat."},
  {5, 2, "Mitsuru, a tall young woman with long, wavy crimson hair falling past her shoulders, a long fringe "
         "sweeping over her left eye, and red eyes. She wears the school uniform: a white high-collared blouse "
         "with full, puffed sleeves and a large red bow tied at the throat."},
  {5, 5, "Mitsuru, a tall young woman with long, wavy crimson hair falling past her shoulders, a long fringe "
         "sweeping over her left eye, and red eyes. She wears casual summer clothes: a sleeveless white ribbed "
         "turtleneck that leaves her shoulders bare."},
  {5, 6, "Mitsuru, a tall young woman with long, wavy crimson hair falling past her shoulders, a long fringe "
         "sweeping over her left eye, and red eyes. She wears casual winter clothes: a black leather "
         "double-breasted coat with wide lapels and silver snap studs, over a cream high-necked shirt whose "
         "collar stands up around her throat."},
  {5, 51, "Mitsuru, a tall young woman with long, wavy crimson hair falling past her shoulders, a long fringe "
          "sweeping over her left eye, and red eyes. She wears the school uniform, a white high-collared blouse "
          "with puffed sleeves and a large red bow at the throat, with a red lettered armband around her upper "
          "arm."},
  {5, 52, "Mitsuru, a tall young woman with long, wavy crimson hair falling past her shoulders, a long fringe "
          "sweeping over her left eye, and red eyes. Over a white puffed-sleeve blouse and large red bow she "
          "wears a black high-collared vest with a strap across the shoulder fastened by a brass snap, like "
          "combat gear."},
  {5, 102, "Mitsuru, a tall young woman with long, wavy crimson hair falling past her shoulders, a long fringe "
           "sweeping over her left eye, and red eyes. She wears a white bikini top with thin shoulder straps, "
           "and a large red hibiscus flower is pinned at her shoulder."},
  {5, 154, "Mitsuru, a tall young woman with red eyes and crimson hair, here gathered up into a high, curling "
           "ponytail with loose strands framing her face. She is wrapped in a pink terry-cloth bath towel tied "
           "above her chest, shoulders bare."},
  {5, 155, "Mitsuru, a tall young woman with red eyes and crimson hair, here gathered up into a high, curling "
           "ponytail. She wears an inn's yukata, a white robe printed with a grey chain-of-circles pattern, "
           "under a loose slate-blue jacket."},
  {5, 156, "Mitsuru, a tall young woman with red eyes and crimson hair, here gathered up into a high, curling "
           "ponytail. She wears a summer festival yukata: white cotton with thin pink stripes, printed with "
           "sprays of green leaves and hanging purple and pink bell-shaped flowers, tied with a green sash."},
  {5, 157, "Mitsuru, a tall young woman with red eyes and crimson hair, pinned up and decorated with a large "
           "white silk flower, red camellias, gold cords and a hanging gold tassel. She wears a formal New Year "
           "kimono: dark green silk with large red camellia blossoms and scattered orange dots, red and gold "
           "patterned collars, and a gold-patterned sash."},
  {5, 159, "Mitsuru, a tall young woman with red eyes and crimson hair, here gathered up into a high, curling "
           "ponytail. She wears a long-sleeved white ribbed turtleneck sweater under a dark brown bib apron with "
           "olive straps."},
  {6, 1, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in her school summer "
         "uniform. A white long-sleeved blouse with a scalloped, lace-edged collar is tied at the front with a "
         "thin white ribbon, over a fitted teal turtleneck."},
  {6, 2, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in her school winter "
         "uniform. A dark charcoal blazer with silver piping, a strap with two round silver buttons across the "
         "chest and a red school crest sits over a high teal turtleneck with a faint lace pattern. A thin "
         "white ribbon is tied at the back of her neck, its ends trailing over one shoulder."},
  {6, 5, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in summer casual "
         "clothes. She wears a sleeveless mint-teal dress with ruffled shoulder straps and a ruffled square "
         "neckline edged with a thin gold line, leaving her arms and collarbones bare."},
  {6, 6, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in winter casual "
         "clothes. A soft green poncho with white embroidered flower motifs has a wide cream collar trimmed "
         "with fur, tied at the front with a white ribbon ending in fluffy pompoms. Underneath is a grey "
         "ribbed turtleneck sweater."},
  {6, 52, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in her support gear "
          "for the Dark Hour. She wears a heavy charcoal greatcoat with a tall stand-up collar fastened by a "
          "strap, brass cross-marked buttons and a short shoulder cape, over a teal top. A silver headset with "
          "large ear cups and a thin microphone arm sits over her hair, and she wears dark fingerless gloves."},
  {6, 102, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in a swimsuit. The top "
           "is plain mint-teal, held by thin white halter straps tied behind her neck, leaving her shoulders and "
           "arms bare."},
  {6, 154, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, wrapped in a pink "
           "terry bath towel. The towel is tucked above her chest, leaving her shoulders and arms bare."},
  {6, 155, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in an inn's robes. A "
           "white cotton yukata printed with grey linked rings is worn under a loose slate-blue haori jacket."},
  {6, 156, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in a summer festival "
           "yukata. It is mid-blue, printed with large yellow sunflowers with brown centres and faint pale green "
           "rings, with an orange obi bow showing behind her back."},
  {6, 157, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in a New Year's "
           "kimono. The kimono is bright teal with white, yellow and red chrysanthemums, its collar layered in "
           "cream, orange and olive, with a pink obi in a red diamond pattern and a yellow bow at the back. "
           "Above one ear she wears an ornament of an orange and a yellow fabric flower with green cords ending "
           "in small silver beads."},
  {6, 901, "Fuuka, shown only as a dark grey silhouette with her features shaded over. The outline shows her "
           "short bob, the high collar and blazer of her winter uniform and the thin ribbon trailing from the "
           "back of her neck."},
  {7, 2, "Aigis, a girl-shaped android with short pale blonde hair and bright blue eyes. A gold headband cut "
         "with triangular openings crosses her hair, white mechanical ear units with red three-lobed centres "
         "replace her ears, and her neck is white with visible seam lines. Here she wears the school winter "
         "uniform: a black blazer with silver piping, a large ruffled red ribbon bow at the throat and a round "
         "red-and-black school badge on the chest."},
  {7, 6, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
         "triangular openings and white mechanical ear units with red centres. In winter casual clothes, she "
         "wears an open teal-blue jacket over a white blouse with a ruffled high collar, a loose white cravat "
         "bow and a frilled front."},
  {7, 52, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
          "triangular openings and white mechanical ear units with red centres. This is her battle form: a "
          "smooth white body with gold, openwork shoulder joints and dark round hinge caps, a large ruffled red "
          "ribbon bow at her throat, and a red armband on her upper arm with a black-and-white emblem and the "
          "letters S.E.E.S."},
  {7, 155, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres; the seams of her white neck "
           "show. She wears an inn's robes: a white cotton yukata printed with grey linked rings under a loose "
           "slate-blue haori jacket."},
  {7, 156, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. She wears a summer festival "
           "yukata in deep blue, printed with large white blossoms with orange centres on slender stems and "
           "leaves, with a red obi bow showing behind her back."},
  {7, 157, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. She wears a New Year's kimono "
           "in pale pink with large cherry blossoms, its collar edged in red and light blue over a gold "
           "patterned underlayer, a green obi and a gold patterned bow at the back. A cluster of pink cherry "
           "blossoms with dangling gold strips is pinned in her hair beside her ear unit."},
  {7, 201, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. Her bare battle body shows: "
           "smooth white plating with seam lines along the neck and chest, and gold, openwork shoulder joints "
           "with dark round hinge caps. She wears no bow or armband."},
  {7, 202, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. Her battle body: smooth white "
           "plating with gold, openwork shoulder joints and dark round hinge caps, and a large ruffled red "
           "ribbon bow at her throat. Unlike her usual battle look, there is no S.E.E.S. armband."},
  {7, 203, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres, in her battle body after heavy "
           "damage. Her white plating, gold shoulder joints, face and hair are smeared with soot and scratches, "
           "and the white trim under her red ribbon bow is torn and ragged."},
  {7, 204, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. She wears a loose pale blue top "
           "like a nightdress, with a white ruffled round collar and a buttoned white placket edged with ruffles "
           "down the front; the seams of her white neck show above it."},
  {8, 1, "Ken, a small boy with a soft, tousled brown bob and large brown eyes. He wears a sleeveless orange "
         "hooded vest, unzipped, over a white long-sleeved collared shirt with a thin black string tie hanging "
         "in loose ends."},
  {8, 2, "Ken, a small boy with a soft, tousled brown bob and large brown eyes. He wears his school winter "
         "uniform: an open dark grey zip jacket with a round red-ringed school badge holding a black-and-white "
         "quartered emblem, over an orange hoodie and a white collared shirt with a thin black string tie."},
  {8, 5, "Ken, a small boy with a soft, tousled brown bob and large brown eyes. He wears casual clothes: a "
         "zip-up track jacket in orange and mustard yellow panels edged with white piping, over a dark grey "
         "T-shirt."},
  {8, 6, "Ken, a small boy with a soft, tousled brown bob and large brown eyes. He wears winter casual "
         "clothes: a thick cream cable-knit sweater with a tall rolled turtleneck, its shoulders and sleeves "
         "in tan and caramel-brown panels."},
  {8, 52, "Ken, a small boy with a soft, tousled brown bob and large brown eyes. He wears his school outfit "
          "ready for battle: a dark grey zip jacket over an orange hoodie and a white collared shirt with a "
          "thin black string tie, a black strap over his shoulder, and a red armband with a black-and-white "
          "emblem around his upper arm."},
  {8, 159, "Ken, a small boy with a soft, tousled brown bob and large brown eyes. He wears a plain yellow "
           "T-shirt with a dark grey neckline under a rust-brown apron with stitched edges and a front pocket, "
           "as if cooking."},
  {9, 6, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears, two grey brow spots, grey shading "
         "on his back and a thick curled tail. He wears a sleeveless tan vest and a grey mechanical collar "
         "with a glowing blue dome on one side and a small white wing fixed behind it."},
  {9, 52, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears, two grey brow spots and a thick "
          "curled tail. In his battle gear he wears the tan vest and grey mechanical collar with a glowing blue "
          "dome and small white wing, plus a black harness with brass-buckled straps, a round brass tag hanging "
          "at his chest, a white canister with red stripes on his back and a red armband on his foreleg."},
  {9, 201, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears, two grey brow spots, grey shading "
           "over his back and a thick curled tail. He wears nothing at all: no collar, vest or gear, just his "
           "fluffy fur with a pale chest."},
  {10, 6, "Shinjiro, a tall young man with shoulder-length, messy brown hair falling over grey eyes, and a long "
          "jaw. He wears a dark grey knit beanie with a small metal plate on the front and a long maroon-red "
          "double-breasted pea coat, buttoned up, with wide dark grey lapels and collar studded with small "
          "silver rivets."},
  {10, 52, "Shinjiro, a tall young man with shoulder-length, messy brown hair falling over grey eyes, and a long "
           "jaw. He wears a dark grey knit beanie with a small metal plate on the front and his long maroon-red "
           "coat with wide studded dark grey lapels, worn open over a black high-necked sweater, a belt buckle "
           "showing at the bottom."},
  {10, 159, "Shinjiro, a tall young man with shoulder-length, messy brown hair falling over grey eyes, and a long "
            "jaw. He wears his dark grey knit beanie with its small metal plate, a plain charcoal long-sleeved "
            "top and a tan canvas apron with stitched edges, as if cooking."},
  {11, 52, "Metis, a slender girl with a short blue-black bob and red eyes. On her head sits a large gold-framed "
           "crown of red butterfly-wing panels with round red ear discs pierced by gold spikes, and it folds "
           "down into a wing-shaped visor over her face. Her body is a seamless dark grey suit with white "
           "jointed shoulder parts, and a red armband with a black-and-white emblem is fixed to her upper arm."},
  {11, 201, "Metis, a slender girl with a short blue-black bob and red eyes. On her head sits a large gold-framed "
            "crown of red butterfly-wing panels with round red ear discs pierced by gold spikes, and it folds "
            "down into a wing-shaped visor over her face. Her body is a seamless dark grey suit with white "
            "jointed shoulder parts; unlike her battle look she wears no armband."},
  {12, 2, "Aigis, a girl-shaped android with short blond hair, blue eyes, a gold lattice headband, "
          "white-and-red mechanical ear units and a white jointed neck. She wears the Gekkoukan winter uniform: "
          "a black blazer with a round red-ringed school badge and a large red bow at her throat."},
  {12, 6, "Aigis, a girl-shaped android with short blond hair, blue eyes, a gold lattice headband, "
          "white-and-red mechanical ear units and a white jointed neck. She wears casual clothes: a white "
          "ruffled high-collared blouse with a large, flowing white cravat bow, under an open teal jacket."},
  {12, 52, "Aigis, a girl-shaped android with short blond hair, blue eyes, a gold lattice headband, "
           "white-and-red mechanical ear units and a white jointed neck. Her robot body shows: a white chest "
           "with a large red bow at the throat, gold mechanical shoulder joints, and a red armband with a "
           "black-and-white emblem on her upper arm."},
  {12, 53, "Aigis, a girl-shaped android with short blond hair, blue eyes, a gold lattice headband, "
           "white-and-red mechanical ear units and a white jointed neck. Her robot body shows: a white chest "
           "with a large red bow at the throat and gold mechanical shoulder joints, with no armband."},
  {12, 54, "Aigis, a girl-shaped android with short blond hair, blue eyes, white-and-red mechanical ear units "
           "and a white jointed neck. She wears armour: a steel visor band edged in gold across her forehead "
           "over the lattice headband, a steel gorget hanging on chains over her red bow, a dark harness lined "
           "with brass bullet cartridges, ribbed gold shoulder armour, and a red armband with a black-and-white "
           "emblem."},
  {12, 201, "Aigis, a girl-shaped android with short blond hair, blue eyes, white-and-red mechanical ear units "
            "and a white jointed neck. She wears armour: a steel visor band edged in gold across her forehead, a "
            "steel gorget on chains over her red bow, a dark harness lined with brass bullet cartridges and "
            "ribbed gold shoulder armour, with no armband."},
  {101, 1, "Kenji, a slim teenage boy with short, messy light-brown hair and brown eyes. He wears the school "
           "summer uniform: a short-sleeved white shirt with a breast pocket and a thin black ribbon tie hanging "
           "loose at the collar."},
  {101, 2, "Kenji, a slim teenage boy with short, messy light-brown hair and brown eyes. He wears the school "
           "winter uniform: a black blazer over a white shirt with a thin black ribbon tie, a small "
           "green-and-yellow pin on the lapel and the round red-and-black school crest on the chest."},
  {101, 6, "Kenji, a slim teenage boy with short, messy light-brown hair and brown eyes. In casual clothes he "
           "wears an open mustard-yellow zip jacket with a high collar over a dark grey T-shirt layered on a "
           "white one."},
  {101, 158, "Kenji, a slim teenage boy with short light-brown hair and brown eyes. He wears a fast-food work "
             "uniform: an orange visor cap with a looping W logo and a short-sleeved orange shirt with a pale "
             "yellow collar, button placket and pocket trim."},
  {102, 1, "Hidetoshi, a lean teenage boy with dark grey hair slicked back from a high forehead, a few strands "
           "falling forward, and narrow grey eyes. He wears the school summer uniform, a short-sleeved white "
           "shirt with a thin black ribbon tie, and a yellow armband with black lettering on his left upper arm."},
  {102, 2, "Hidetoshi, a lean teenage boy with dark grey hair slicked back from a high forehead and narrow grey "
           "eyes. He wears the school winter uniform, a black blazer with the school crest and a small lapel pin "
           "over a white shirt and thin black ribbon tie, with a yellow armband with black lettering over the "
           "blazer sleeve."},
  {103, 6, "Bunkichi, an elderly man with wild, thinning white hair, bushy white eyebrows, a white moustache and "
           "black rectangular glasses, with a dark mark on his bald forehead. He wears a cream knitted sweater "
           "with a dark ribbed turtleneck collar and a red and blue zigzag pattern across the chest and arms, "
           "and holds a dark wooden tobacco pipe."},
  {104, 6, "Mitsuko, a small, very old woman with thinning grey hair cut to chin length and a heavily wrinkled "
           "round face. She wears a lilac blouse printed with grey leaves and small flowers under a dark "
           "olive-green work apron held by shoulder straps with round brass buttons."},
  {105, 101, "Kazushi, an athletic teenage boy with short, spiky dark grey hair cut close at the sides and dark "
             "eyes. He wears a sporty track jacket in grey with white stripes down the front and shoulders, red "
             "panels at the chest, and a high zipped collar."},
  {105, 151, "Kazushi, an athletic teenage boy with short, spiky dark grey hair cut close at the sides and dark "
             "eyes. He wears a dark grey sleeveless athletic vest edged in red, leaving his shoulders and arms "
             "bare."},
  {106, 1, "Yuko, a teenage girl with tanned skin, short dark blue-black hair with a small ponytail sticking up "
           "at the back, a pink hairpin, and brown eyes. She wears the school summer uniform: a short-sleeved "
           "white blouse with a breast pocket and a large red bow at the collar."},
  {106, 2, "Yuko, a teenage girl with tanned skin, short dark blue-black hair with a small ponytail at the back, "
           "a pink hairpin, and brown eyes. She wears the school winter uniform: a black jacket with silver "
           "buttons and the round school crest over a white blouse with a large red bow."},
  {106, 6, "Yuko, a teenage girl with tanned skin, short dark blue-black hair with a small ponytail at the back, "
           "a pink hairpin, and brown eyes. In casual clothes she wears an open black zip-up hoodie with a white "
           "hood lining and white stripes down the sleeves over a coral-red top."},
  {106, 101, "Yuko, a teenage girl with tanned skin, short dark blue-black hair with a small ponytail at the back, "
             "a pink hairpin, and brown eyes. She wears a sporty grey track jacket zipped up to a high collar, "
             "with white stripes down the front and shoulders and red panels on the chest."},
  {107, 6, "Maya, an online game avatar drawn as a translucent green hologram made of flat, low-polygon shapes "
           "with horizontal scan lines and a pink and cyan glow at the edges. The figure has a short, "
           "flicked-out bob of hair, one visible eye, and a bulky high-collared coat with stripes and diamond "
           "shapes across it."},
  {108, 1, "Keisuke, a slim teenage boy with soft, wavy light-brown hair and grey eyes behind black rectangular "
           "glasses. He wears the school summer uniform: a short-sleeved white shirt with a breast pocket and a "
           "thin black ribbon tie."},
  {108, 2, "Keisuke, a slim teenage boy with soft, wavy light-brown hair and grey eyes behind black rectangular "
           "glasses. He wears the school winter uniform: a black blazer with a small lapel pin and the round "
           "school crest over a white shirt and thin black ribbon tie."},
  {108, 6, "Keisuke, a slim teenage boy with soft, wavy light-brown hair and grey eyes behind black rectangular "
           "glasses. In casual clothes he wears a red, blue and white plaid button-up shirt over a dark green "
           "T-shirt."},
  {109, 1, "Chihiro, a slender teenage girl with very long, straight brown hair parted in the middle and falling "
           "past her shoulders, and brown eyes behind black rectangular glasses. She wears the school summer "
           "uniform: a white blouse with a large red bow at the collar."},
  {109, 2, "Chihiro, a slender teenage girl with very long, straight brown hair parted in the middle, and brown "
           "eyes behind black rectangular glasses. She wears the school winter uniform: a black jacket over a "
           "white blouse with a large red bow."},
  {109, 6, "Chihiro, a slender teenage girl with very long, straight brown hair parted in the middle, and brown "
           "eyes behind black rectangular glasses. In casual clothes she wears a soft tan cardigan with a "
           "ruffled button front over a cream top with a zigzag-edged high collar."},
  {110, 6, "Maiko, a little girl with brown hair pulled back into a braided bun held by a pink scrunchie, and "
           "big brown eyes. She wears an open denim-blue shirt with silver buttons over a pink T-shirt, and a "
           "red leather school backpack on her shoulders."},
  {111, 6, "Pharos, a small, pale boy with messy dark blue-grey hair, large bright blue eyes and a small mole "
           "under his left eye. He wears a long-sleeved shirt with wide grey and white horizontal stripes and a "
           "short buttoned placket at the neck."},
  {112, 1, "Bebe, a teenage boy with a blond bowl-cut and bangs, light skin and pale blue eyes. He wears the "
           "school summer uniform, a short-sleeved white shirt with a thin black ribbon tie, and holds an open "
           "black folding fan painted with gold butterflies."},
  {112, 2, "Bebe, a teenage boy with a blond bowl-cut and bangs, light skin and pale blue eyes. He wears the "
           "school winter uniform, a black blazer with the round school crest and a small lapel pin over a white "
           "shirt and black ribbon tie, and holds an open black folding fan painted with gold butterflies."},
  {112, 6, "Bebe, a teenage boy with a blond bowl-cut and bangs, light skin and pale blue eyes. In casual "
           "clothes he wears a dark charcoal long-sleeved T-shirt with a faint grey dragon printed on the chest."},
  {113, 6, "President Tanaka, a middle-aged man with thick, wavy dark grey hair and heavy-lidded eyes. He wears "
           "a grey-green suit jacket over a white shirt and a dark red tie."},
  {114, 6, "Mutatsu, an elderly, completely bald man with a deeply lined face and heavy brow. He wears a "
           "Buddhist monk's robes: a dark charcoal kimono-style robe over white under-layers, with a pale cord "
           "across the chest, and he holds a lit cigar, gold rings on his fingers."},
  {114, 901, "Mutatsu, an elderly, completely bald man with a deeply lined face and heavy brow. He wears a "
             "Buddhist monk's robes: a dark charcoal kimono-style robe over white under-layers with a pale cord "
             "across the chest. Unlike his usual look, he holds no cigar."},
  {115, 6, "Mamoru, an athletic teenage boy with short, spiky dark grey hair and grey eyes. He wears a light "
           "teal zip-up hoodie with drawstrings, a small dark triangle logo on the chest, and black-and-white "
           "stripes printed with triangles down the sleeves."},
  {116, 1, "Nozomi, a heavyset teenage boy with a round face, chubby cheeks, short black hair with a curl "
           "sticking up on top and drooping eyes. He wears the school summer uniform: a short-sleeved white "
           "shirt with a breast pocket and a thin black ribbon tie."},
  {116, 2, "Nozomi, a heavyset teenage boy with a round face, chubby cheeks, short black hair with a curl "
           "sticking up on top and drooping eyes. He wears the school winter uniform: a black blazer with the "
           "round school crest and a small lapel pin over a white shirt and thin black ribbon tie."},
  {117, 6, "Akinari, a thin, pale young man with straight, chin-length silver-grey hair and grey eyes. He wears "
           "a loose button-up shirt with bold grey-blue and white vertical stripes, open at the neck."},
  {201, 6, "Igor, a short, wizened old man with a bald dome of a head, fringes of white hair at the back, "
           "pointed ears, bulging bloodshot eyes and an extremely long, pointed nose. He wears a black suit with "
           "white gloves."},
  {202, 6, "Elizabeth, a young woman with a short silver bob with straight bangs, and bright yellow eyes. She "
           "wears her Velvet Room attendant uniform: a small blue cap tilted on her head, a blue sleeveless "
           "dress with a tall collar and a large dark circle with a yellow centre on the chest, and long blue "
           "gloves."},
  {202, 901, "Elizabeth, in her usual blue Velvet Room uniform, cap and long gloves, wearing a pair of chunky "
             "black-rimmed glasses over her yellow eyes."},
  {202, 902, "Elizabeth, in her usual blue Velvet Room uniform and cap, wearing white cat ears on a band and a "
             "white cat-muzzle mask with a pink nose and whiskers over her mouth and nose."},
  {202, 903, "Elizabeth, in her usual blue Velvet Room uniform and cap, wearing joke novelty glasses: swirly round "
             "lenses, a big pink plastic nose and a black curled moustache."},
  {202, 904, "Elizabeth, in her usual blue Velvet Room uniform and long gloves, wearing a red Santa hat with white "
             "fur trim and a pom-pom in place of her cap, and a fluffy white false beard and moustache."},
  {211, 6, "Takaya, a gaunt man with very long, wavy silver-grey hair held by a thin dark circlet around his "
           "forehead, yellow eyes and light stubble. He is bare-chested, with purple tattoos on both shoulders, "
           "a crowned serpent and a sun-like spiral on the right."},
  {212, 6, "Jin, a young man with straight, dark blue hair falling over one side of his face, and orange-tinted "
           "rectangular glasses. He wears a green jacket zipped up to a tall collar, with grey stripes down the "
           "front, black straps over the shoulders and a black band around the upper arm."},
  {213, 6, "Chidori, a pale young woman with very long, straight red hair with blunt bangs and grey eyes with "
           "dark lashes. She wears a frilly white gothic-lolita dress with ruffles, a ribbon choker and ribbons "
           "laced through her hair, a black and white ruffled headdress studded with silver diamonds, and a "
           "large silver dagger-shaped pin through it."},
  {213, 201, "Chidori, a pale young woman with very long, straight red hair with blunt bangs and grey eyes, "
             "without her headdress or ribbons. She wears a plain pale blue hospital gown with a wrap-over front."},
  {221, 2, "Ryoji, a slender teenage boy with dark hair swept back with a few strands sticking up, blue eyes, "
           "and a small beauty mark under his left eye. He wears the school winter uniform, a white shirt with "
           "an open dark jacket, and a long pale-yellow scarf wrapped around his neck and hanging down the "
           "front."},
  {221, 154, "Ryoji, a slender teenage boy with dark hair swept back with a few strands sticking up, blue eyes, "
             "and a small beauty mark under his left eye. He is bare-chested and bare-shouldered, as at a bath or "
             "hot spring."},
  {221, 155, "Ryoji, a slender teenage boy with dark hair swept back with a few strands sticking up, blue eyes, "
             "and a small beauty mark under his left eye. He wears an inn-style yukata: a grey-blue outer robe "
             "over a white inner robe patterned with grey chain-like circles."},
  {222, 6, "Ikutsuki, a lanky middle-aged man with long, wavy brown hair pulled back from a receding hairline, "
           "thin wire-rimmed glasses and a small goatee. He wears a tan double-breasted coat over a dark grey "
           "ribbed turtleneck sweater."},
  {223, 1, "Natsuki, a teenage girl with tanned skin, long wavy auburn hair with straight bangs and a short "
           "ponytail tied high at the back, and brown eyes. She wears the school summer uniform: a white blouse "
           "with a wide collar, a grey knitted sweater vest and a large red bow."},
  {223, 2, "Natsuki, a teenage girl with tanned skin, long wavy auburn hair with straight bangs and a short high "
           "ponytail, and brown eyes. She wears the school winter uniform with a dark grey cardigan over a white "
           "blouse with a wide collar and a large red bow."},
  {223, 6, "Natsuki, a teenage girl with tanned skin, auburn hair with straight bangs and a short high ponytail, "
           "here falling to her shoulders, and brown eyes. In casual clothes she wears an oversized cream hooded "
           "jacket over a pink top and a silver heart-shaped pendant on a chain."},
  {224, 6, "Takeharu, a stern middle-aged man with dark hair swept to one side and a black eyepatch over his "
           "right eye. He wears a grey three-piece suit with a dark waistcoat, a white shirt, a dark red tie and "
           "a small gold pin on the lapel."},
  {252, 1, "Akihiko as a young boy, with short, tousled silver hair, grey eyes and a bandage across the bridge "
           "of his nose. He wears a white short-sleeved button-up shirt, open at the neck, over a red T-shirt."},
  {253, 1, "Mitsuru as a younger teenager, with wavy red hair to her shoulders held back by a purple hairband, "
           "and red eyes. She wears a school uniform of a grey knitted sweater vest over a white blouse with a "
           "large red bow at the collar."},
  {254, 5, "Mitsuru as a little girl, with red hair worn in long ringlet drill curls on both sides, and red "
           "eyes. She wears a white blouse with fine vertical pleats and buttons, and a large red bow at the "
           "collar fastened with a dark oval cameo brooch."},
  {1081, 6, "Mr. Edogawa, a lanky man with messy dark hair, thick black rectangular glasses, stubble and a red "
            "pencil tucked behind his ear. He wears a rumpled white lab coat over a mustard-yellow T-shirt with a "
            "dark collar and a white hand-sign print."},
  {1082, 6, "A middle-aged man with neat grey hair falling to his brows and lined cheeks. He wears a grey suit "
            "jacket, a white shirt and a purple and grey diagonally striped tie."},
  {1083, 6, "Mr. Ono, a middle-aged man with a goatee who wears a dark iron samurai helmet with a curved gold "
            "crescent crest, tied under his chin. Below it he wears a brown plaid suit jacket, a white shirt and "
            "a red tie."},
  {1084, 6, "A man in his thirties with tousled dark grey hair parted in the middle and light stubble. He wears a "
            "dark grey suit jacket, a white shirt and a red tie with a diamond check pattern."},
  {1085, 6, "Mr. Miyahara, a slight man with a huge round grey afro, small round wire-rimmed glasses and a "
            "prominent front tooth. He wears a mustard-yellow V-neck sweater over a green pinstriped shirt with a "
            "tall collar, and carries a large wooden set-square ruler."},
  {1086, 6, "A police officer, a hard-faced man with short dark hair under a peaked police cap with a gold star "
            "badge. He wears a navy police uniform jacket with a white shirt and dark tie, a badge on the breast "
            "and a radio microphone clipped at the shoulder."},
  {1087, 6, "An old man with long blue-grey hair tied back in a ponytail, a balding crown and a full, shaggy "
            "blue-grey beard. He wears a white high-collared jacket with brass buttons, like a cook's coat."},
  {1181, 6, "A heavyset older man with grey hair combed back, a round face with a double chin, small round "
            "wire-rimmed glasses and a bushy grey moustache. He wears a dark grey suit jacket over a beige "
            "knitted V-neck vest, a white shirt and a red tie."},
  {1581, 6, "Ms. Toriumi, a young woman with a chin-length, wavy light-brown bob, brown eyes and small pearl stud "
            "earrings. She wears a dusty rose suit jacket over a white blouse with a high buttoned collar."},
  {1582, 6, "A woman with brown hair swept to one side and pinned up at the back, brown eyes behind red-framed "
            "glasses, and a pearl necklace. She wears a white jacket with a wide collar over a lilac top."},
  {1583, 6, "Mrs. Terauchi, a woman with shoulder-length brown hair flipped out at the ends and held by a yellow "
            "hairband, and brown eyes. She wears an orange suit jacket over a white top with a yellow silk scarf "
            "tied at her throat."},
  {1586, 6, "A woman with long, wavy black hair pulled back loosely, green eyes and thin rimless glasses worn low "
            "on her nose. She wears a white top with a high mandarin collar trimmed in green and a thin gold "
            "choker ring."},
};
const Pose kPoses[] = {
  {1, 0, 'A', "His head tilts to one side, turned slightly away, looking out from under his fringe."},
  {2, 1, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 1, 'B', "She faces forward with her head nearly level."},
  {2, 2, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 2, 'B', "She faces forward with her head nearly level."},
  {2, 5, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 5, 'B', "She faces forward with her head nearly level."},
  {2, 6, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 6, 'B', "She faces forward with her head nearly level."},
  {2, 51, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 51, 'B', "She faces forward with her head nearly level."},
  {2, 52, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 52, 'B', "She faces forward with her head nearly level."},
  {2, 102, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 102, 'B', "She faces forward with her head nearly level."},
  {2, 106, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 154, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 154, 'B', "She faces forward with her head nearly level."},
  {2, 155, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 155, 'B', "She faces forward with her head nearly level."},
  {2, 156, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 156, 'B', "She faces forward with her head nearly level."},
  {2, 157, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 159, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 159, 'B', "She faces forward with her head nearly level."},
  {2, 201, 'A', "Her head tilts to one side, chin lowered, looking out at an angle."},
  {2, 201, 'B', "She faces forward with her head nearly level."},
  {3, 1, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 1, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 1, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 1, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 2, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 2, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 2, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 2, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 5, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 5, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 5, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 5, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 6, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 6, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 6, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 6, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 51, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 51, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 51, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 51, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 52, 'A', "He stands turned slightly away, the sword hilt over his shoulder, head tilted to look off sideways."},
  {3, 52, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 52, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 52, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 102, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 102, 'B', "His head is bowed forward and tilted to one side."},
  {3, 102, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 102, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 154, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 154, 'B', "His head is bowed forward and tilted to one side."},
  {3, 154, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 154, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 155, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 155, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 155, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 155, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 202, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 202, 'B', "His head is bowed forward and tilted, the cap's brim low over one eye."},
  {3, 202, 'C', "His head is tipped back with his chin raised, looking upward."},
  {3, 202, 'D', "He leans forward with his body turned sideways and his head jerked back."},
  {3, 901, 'A', "He stands with his shoulders square, head tilted a little to one side, looking off sideways."},
  {3, 901, 'C', "His head is tipped back with his chin raised, looking upward."},
  {4, 1, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 1, 'B', "He faces forward with his head upright."},
  {4, 2, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 2, 'B', "He faces forward with his head upright."},
  {4, 5, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 5, 'B', "He faces forward with his head upright."},
  {4, 6, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 6, 'B', "He faces forward with his head upright."},
  {4, 51, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 51, 'B', "He faces forward with his head upright."},
  {4, 52, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 52, 'B', "He faces forward with his head upright."},
  {4, 102, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 102, 'B', "He faces forward with his head upright."},
  {4, 154, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 154, 'B', "He faces forward with his head upright."},
  {4, 155, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 155, 'B', "He faces forward with his head upright."},
  {4, 159, 'A', "His head tilts to one side and his body is turned three-quarters away, glancing sideways."},
  {4, 159, 'B', "He faces forward with his head upright."},
  {5, 1, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways over her shoulder."},
  {5, 1, 'B', "She faces forward with her head upright."},
  {5, 2, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways over her shoulder."},
  {5, 2, 'B', "She faces forward with her head upright."},
  {5, 5, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways over her shoulder."},
  {5, 5, 'B', "She faces forward with her head upright."},
  {5, 6, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways over her shoulder."},
  {5, 6, 'B', "She faces forward with her head upright."},
  {5, 51, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways over her shoulder."},
  {5, 51, 'B', "She faces forward with her head upright."},
  {5, 52, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways over her shoulder."},
  {5, 52, 'B', "She faces forward with her head upright."},
  {5, 102, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways over her shoulder."},
  {5, 102, 'B', "She faces forward with her head upright."},
  {5, 154, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways."},
  {5, 155, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways."},
  {5, 156, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways."},
  {5, 157, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways."},
  {5, 159, 'A', "Her body is turned three-quarters away and her head tilts down, looking sideways."},
  {6, 1, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 1, 'B', "She faces forward with one hand raised in a loose fist against her chest."},
  {6, 2, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 2, 'B', "She faces forward with one hand raised in a loose fist against her chest."},
  {6, 5, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 5, 'B', "She faces forward with one hand raised in a loose fist against her chest."},
  {6, 6, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 6, 'B', "She faces forward with one hand raised to her chest, holding the ribbon of her collar."},
  {6, 52, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 52, 'B', "She faces forward with one gloved hand raised in a loose fist against her chest."},
  {6, 102, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 102, 'B', "She faces forward with one hand raised in a loose fist against her chest."},
  {6, 154, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 154, 'B', "She faces forward with one hand raised in a loose fist against her chest, holding the towel."},
  {6, 155, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 155, 'B', "She faces forward with one hand raised in a loose fist against her chest."},
  {6, 156, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 156, 'B', "She faces forward with one hand raised in a loose fist against her chest."},
  {6, 157, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {6, 901, 'A', "Her head tilts slightly to one side and her arms rest down at her sides."},
  {7, 2, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 2, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 6, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 6, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 52, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 52, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 155, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 155, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 156, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 156, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 157, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 201, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 201, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 202, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 202, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 203, 'B', "She faces straight ahead, arms down at her sides."},
  {7, 204, 'A', "Her body is turned a little to one side, head slightly tilted, arms down at her sides."},
  {7, 204, 'B', "She faces straight ahead, arms down at her sides."},
  {8, 1, 'A', "His head tilts to one side as he faces forward."},
  {8, 1, 'B', "His head is bowed, and he looks up from under his bangs."},
  {8, 2, 'A', "His head tilts to one side as he faces forward."},
  {8, 2, 'B', "His head is bowed, and he looks up from under his bangs."},
  {8, 5, 'A', "His head tilts to one side as he faces forward."},
  {8, 5, 'B', "His head is bowed, and he looks up from under his bangs."},
  {8, 6, 'A', "His head tilts to one side as he faces forward."},
  {8, 6, 'B', "His head is bowed, and he looks up from under his bangs."},
  {8, 52, 'A', "His head tilts to one side as he faces forward."},
  {8, 52, 'B', "His head is bowed, and he looks up from under his bangs."},
  {8, 159, 'A', "His head tilts to one side as he faces forward."},
  {8, 159, 'B', "His head is bowed, and he looks up from under his bangs."},
  {9, 6, 'A', "He sits upright, head raised and turned to the side, ears pricked."},
  {9, 6, 'B', "He sits with his head lowered and his ears folded back."},
  {9, 6, 'C', "He hunches forward with his head low and ears up, muzzle thrust out."},
  {9, 6, 'D', "He sits with his head thrown back and his nose pointed up."},
  {9, 52, 'A', "He sits upright, head raised and turned to the side, ears pricked."},
  {9, 52, 'B', "He sits with his head lowered and his ears folded back."},
  {9, 52, 'C', "He hunches forward with his head low and ears up, muzzle thrust out."},
  {9, 52, 'D', "He sits with his head thrown back and his nose pointed up."},
  {9, 201, 'A', "He sits upright, head raised and turned to the side, ears pricked."},
  {9, 201, 'B', "He sits with his head lowered and his ears folded back."},
  {9, 201, 'C', "He hunches forward with his head low and ears up, muzzle thrust out."},
  {9, 201, 'D', "He sits with his head thrown back and his nose pointed up."},
  {10, 6, 'A', "He leans forward, shoulders hunched, eyeing sideways from under his beanie."},
  {10, 6, 'B', "He straightens with his head raised and turned slightly to the side."},
  {10, 52, 'A', "He leans forward, shoulders hunched, eyeing sideways from under his beanie."},
  {10, 52, 'B', "He straightens with his head raised and turned slightly to the side."},
  {10, 159, 'A', "He leans forward, shoulders hunched, eyeing sideways from under his beanie."},
  {11, 52, 'A', "Her visor is raised and her face shows, head tilted slightly."},
  {11, 52, 'B', "Her wing-shaped visor is lowered over her face, her head dipped slightly."},
  {11, 52, 'C', "Her wing-shaped visor is lowered over her face, her head dipped slightly."},
  {11, 201, 'A', "Her visor is raised and her face shows, head tilted slightly."},
  {11, 201, 'B', "Her wing-shaped visor is lowered over her face, her head dipped slightly."},
  {11, 201, 'C', "Her wing-shaped visor is lowered over her face, her head dipped slightly."},
  {12, 2, 'A', "Her head is turned and tilted slightly to one side."},
  {12, 2, 'B', "She faces straight ahead, head level."},
  {12, 6, 'A', "Her head is turned and tilted slightly to one side."},
  {12, 6, 'B', "She faces straight ahead, head level."},
  {12, 52, 'A', "Her head is turned and tilted slightly to one side."},
  {12, 52, 'B', "She faces straight ahead, head level."},
  {12, 53, 'A', "Her head is turned and tilted slightly to one side."},
  {12, 53, 'B', "She faces straight ahead, head level."},
  {12, 54, 'A', "Her head is turned and tilted slightly to one side."},
  {12, 54, 'B', "She faces straight ahead, head level."},
  {12, 201, 'A', "Her head is turned and tilted slightly to one side."},
  {12, 201, 'B', "She faces straight ahead, head level."},
  {101, 1, 'A', "He stands turned slightly to his left, head tilted a little."},
  {101, 2, 'A', "He stands turned slightly to his left, head tilted a little."},
  {101, 6, 'A', "He stands turned slightly to his left, head tilted a little."},
  {101, 158, 'A', "He stands turned slightly to his left, head tilted a little."},
  {102, 1, 'A', "He stands with his body angled to his left and his head tipped slightly."},
  {102, 2, 'A', "He stands with his body angled to his left and his head tipped slightly."},
  {103, 6, 'A', "He leans forward, stooped, holding the pipe out in his right hand."},
  {104, 6, 'A', "She stands slightly hunched, facing forward."},
  {105, 101, 'A', "His body is turned to his right, his head facing forward with a slight tilt."},
  {105, 151, 'A', "His body is turned to his right, his head facing forward with a slight tilt."},
  {106, 1, 'A', "She faces forward, head tilted slightly to one side."},
  {106, 2, 'A', "She faces forward, head tilted slightly to one side."},
  {106, 6, 'A', "She faces forward, head tilted slightly to one side."},
  {106, 101, 'A', "She faces forward, head tilted slightly to one side."},
  {107, 6, 'A', "The avatar faces forward and holds up one open hand, palm up, beside its shoulder."},
  {108, 1, 'A', "He faces forward with his head tilted slightly to his left."},
  {108, 2, 'A', "He faces forward with his head tilted slightly to his left."},
  {108, 6, 'A', "He faces forward with his head tilted slightly to his left."},
  {109, 1, 'A', "She holds her right hand loosely at her chest, near the bow."},
  {109, 2, 'A', "She holds her right hand loosely at her chest, near the bow."},
  {109, 6, 'A', "She holds her right hand loosely at her chest."},
  {110, 6, 'A', "She grips both backpack straps in front of her chest."},
  {111, 6, 'A', "He faces straight ahead, arms at his sides."},
  {112, 1, 'A', "He holds the open fan up in his right hand beside his chin."},
  {112, 2, 'A', "He holds the open fan up in his right hand beside his chin."},
  {112, 6, 'A', "He stands with his arms folded low in front of him, head tilted."},
  {113, 6, 'A', "His body is turned to his right, his head facing forward."},
  {114, 6, 'A', "He holds the cigar up near his chin in his right hand, head raised."},
  {114, 901, 'A', "He looks upward and to his right, hands out of view."},
  {115, 6, 'A', "He faces forward, shoulders square, head turned slightly."},
  {115, 6, 'P', "His face appears on a phone screen in blue, his name 'Mamoru Hayase' across the top, as on an "
                "incoming call."},
  {116, 1, 'A', "He stands turned to his right, head tilted down slightly."},
  {116, 2, 'A', "He stands turned to his right, head tilted down slightly."},
  {117, 6, 'A', "His head is turned and raised, looking up and off to his left."},
  {201, 6, 'A', "He hunches forward, gloved hands crossed under his chin, elbows resting."},
  {202, 6, 'A', "She holds up her left hand, palm open and up, beside her shoulder."},
  {202, 6, 'P', "Her face appears on a blue phone screen in dark silhouette, the caller name reading 'Unknown', as on "
                "an incoming call."},
  {202, 901, 'A', "She holds up her left hand, palm open and up, beside her shoulder."},
  {202, 902, 'A', "She holds up her left hand, palm open and up, beside her shoulder."},
  {202, 903, 'A', "She holds up her left hand, palm open and up, beside her shoulder."},
  {202, 904, 'A', "She holds up her left hand, palm open and up, beside her shoulder."},
  {211, 6, 'A', "He faces straight ahead, head level."},
  {211, 6, 'B', "He faces forward with his head drawn back and shoulders raised, his hair fanning out wider."},
  {212, 6, 'A', "His body is turned to his left, head turned back toward the front."},
  {213, 6, 'A', "She stands turned slightly, head tilted to one side."},
  {213, 201, 'A', "She stands turned slightly, head tilted to one side."},
  {221, 2, 'A', "He stands turned slightly, head tilted to one side."},
  {221, 154, 'A', "He stands turned slightly, head tilted to one side."},
  {221, 155, 'A', "He stands turned slightly, head tilted to one side."},
  {222, 6, 'A', "He faces forward, head tilted slightly to his left."},
  {222, 6, 'B', "He leans his head back with his eyes shut and chin raised."},
  {222, 6, 'P', "His face appears on a phone screen in sepia tones, his name 'Shuji Ikutsuki' across the top, as on "
                "an incoming call."},
  {223, 1, 'A', "Her body is turned away to her right, head turned back over her shoulder."},
  {223, 2, 'A', "Her body is turned away to her right, head turned back over her shoulder."},
  {223, 6, 'A', "Her body is turned away to her right, head turned back over her shoulder."},
  {224, 6, 'A', "He faces forward, head turned slightly to his right."},
  {252, 1, 'A', "He stands turned slightly, head tilted to one side."},
  {253, 1, 'A', "She faces forward, head tilted slightly to her left."},
  {254, 5, 'A', "She faces forward, shoulders square, head level."},
  {1081, 6, 'A', "He rubs the side of his neck with his right hand, head tilted."},
  {1082, 6, 'A', "His body is turned to his left, head tilted toward the front."},
  {1083, 6, 'A', "His body is turned to his left, head turned back to the right."},
  {1084, 6, 'A', "His body is turned to his left, head turned back toward the front."},
  {1085, 6, 'A', "He rests the set square on his shoulder, holding it with his right hand."},
  {1086, 6, 'A', "His body is turned to his left, his cap brim shading one eye."},
  {1087, 6, 'A', "His body is turned to his left, head raised and looking to the right."},
  {1181, 6, 'A', "His body is turned to his left, head raised."},
  {1581, 6, 'A', "She faces forward, head tilted slightly to her right."},
  {1582, 6, 'A', "She faces forward, head tilted slightly to her right."},
  {1583, 6, 'A', "Her arms are folded across her waist, head tilted to her right."},
  {1586, 6, 'A', "She faces forward, head tilted to her right, looking over her glasses."},
};
