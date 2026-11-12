// The dialogue portraits (included by portrait_text.cpp): every outfit of every character the game has a
// bust-up for (T_BU_<PC|SC|NC><char>_Pose<x>_C<outfit>, 156 outfits in 267 poses), each rendered from the game's own
// textures and looked at (tools/bustup/outfits.py, 2026-11-12). An outfit's text is the character's look in it; a
// pose's text is said after it. Character ids: 1 protagonist, 2 Yukari, 3 Junpei, 4 Akihiko, 5 Mitsuru, 6 Fuuka,
// 7 Aigis, 8 Ken, 9 Koromaru, 10 Shinjiro, 11 Metis, 12 Aigis (Episode Aigis); 1xx Social Links, 2xx story, 1xxx others.
const Outfit kOutfits[] = {
  {1, 0, "The protagonist, a slim, pale-skinned teenage boy with soft, dark slate-blue hair cut in a rounded "
         "bob, a tuft sticking up at the crown and a long fringe falling across his right eye, so only his "
         "grey-blue left eye shows clearly. He wears a plain dark charcoal-grey top of smooth fabric with a "
         "small upright collar split into a narrow V at the throat; it has no buttons, prints or trim. No "
         "jewellery or headphones are visible, and the portrait ends at his upper chest."},
  {2, 1, "Yukari, a slim teenage girl with short, chin-length light brown hair that flips outward at the ends "
         "and brown eyes, wearing small silver stud earrings. She wears a pale grey choker with a heart-shaped "
         "buckle and a white short-sleeved button-up blouse, collar open, with a breast pocket: her school "
         "summer uniform."},
  {2, 2, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
         "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. Around her neck is "
         "a pale grey leather choker with stitched edges and a heart-shaped buckle. She wears her school "
         "uniform: a soft pink ribbed cardigan, buttoned, over a white shirt with a wide, sharply pointed "
         "collar standing out over the cardigan, and a large bright red ribbon bow tied at the throat with its "
         "tails hanging down."},
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
  {2, 102, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. She wears a "
           "swimsuit: a plain pale pink bikini top with thin teal straps edged in a darker line, tied in a "
           "halter behind her neck and meeting at a small knot between her cups; her shoulders and arms are "
           "bare. Pushed up on top of her head are sunglasses with thick, dark slate-grey rectangular frames, "
           "smoky brown lenses and small gold accents on the arms. She is not wearing her usual choker."},
  {2, 106, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes and small silver stud earrings. She wears a maid outfit: on "
           "her head a black headband carrying a tall white frilled headdress of gathered lace, its scalloped "
           "edge punched with little eyelet holes. Her dress is charcoal black with puffed shoulders, and down "
           "its front runs a rounded white bib with a tall, sharply pointed white collar, fastened by a row of "
           "four round black buttons. She is not wearing her usual choker."},
  {2, 154, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. She is wrapped in a "
           "dusty pink terry-cloth bath towel tucked straight across her chest, its fluffy top edge just above "
           "her bust, as at a hot spring bath; her shoulders, collarbones and arms are bare. She is not wearing "
           "her usual choker, and the portrait ends at the top of the towel."},
  {2, 155, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes and small silver stud earrings. She wears an inn's robes: a "
           "white cotton yukata crossed left over right at the neck, printed with a chain of grey outlined "
           "rings, each ring joined to the next by a small dark grey dot, with thin grey lines along the collar "
           "edges. Over it hangs a loose, wide-sleeved slate-blue haori jacket, worn open, of plain matte cloth. "
           "She is not wearing her usual choker."},
  {2, 156, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes and small silver stud earrings. She wears a summer festival "
           "yukata in a large pink and white checkerboard, crossed at the neck, printed with small five-petalled "
           "flowers in yellow, pink and orange-red on slender green stems with long leaves. A glimpse of a tan "
           "sash shows at her back, near the bottom of the picture. She is not wearing her usual choker."},
  {2, 157, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes. She wears a "
           "formal New Year's kimono in deep pink printed with large white cherry blossoms, a green-edged collar "
           "and a teal patterned sash, and a hair ornament of pink and white flowers with strands of dangling "
           "pink and silver petals over her left ear."},
  {2, 159, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. She wears her pale "
           "grey leather choker with stitched edges and a heart-shaped buckle. Under it is a coral-red "
           "long-sleeved crew-neck top, covered by a work apron striped vertically in pink and white, with dark "
           "red straps over her shoulders, white stitching along its top edge and straps, and a stitched front "
           "pocket."},
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
  {4, 5, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
         "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
         "above his left eyebrow. He wears casual summer clothes: a deep red V-neck shirt in a soft knit with "
         "thin pale vertical pinstripes, a darker red band across the top of the shoulder, open at the throat "
         "to show his collarbones. No jewellery is visible."},
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
  {4, 102, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He wears a plain white crew-neck T-shirt with a ribbed neckband, printed on "
           "the chest in grey: small lettering reading \"SCUBA DIVING\" above a large bold \"SOS\". No jewellery "
           "is visible, and the portrait ends just below the lettering."},
  {4, 154, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He is shirtless, as at a bath or hot spring, showing a thick neck, broad "
           "bare shoulders, sharp collarbones and the top of a muscular chest. Nothing else is worn, no "
           "jewellery or towel shows, and the portrait ends at his upper chest."},
  {4, 155, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He wears an inn's yukata: a white cotton robe crossed loosely at the neck so "
           "his collarbones show, printed with a chain of grey outlined rings joined by small dark grey dots, "
           "with thin grey lines along the collar. Over it hangs a loose, wide-sleeved slate-blue haori jacket, "
           "worn open."},
  {4, 159, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He wears a white collarless shirt with a narrow band neck and a short open "
           "slit at the throat, marked with faint vertical pinstripes. Over it is a dark red bib apron with "
           "straps over his shoulders, its edges and front pocket outlined in dashed stitching."},
  {5, 1, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair falling past her "
         "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
         "eyes and soft coral-pink lipstick. She wears the school uniform: a white blouse with a high, "
         "close-fitting collar, full puffed sleeves and fine vertical pleats down the front, and a large red "
         "ribbon bow tied at the throat with wide tails hanging down her chest. No jewellery is visible."},
  {5, 2, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair falling past her "
         "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
         "eyes and soft coral-pink lipstick. She wears the school uniform: a white blouse with a high, "
         "close-fitting collar, full puffed sleeves and fine vertical pleats down the front, and a large red "
         "ribbon bow tied at the throat with wide tails hanging down her chest. No jewellery is visible."},
  {5, 5, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair falling past her "
         "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
         "eyes and soft coral-pink lipstick. She wears casual summer clothes: a sleeveless white turtleneck in "
         "a fine vertical rib knit, its tall neck folded over, cut in deeply at the armholes so her shoulders "
         "and arms are bare. It has no print or trim, and no jewellery is visible."},
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
  {5, 102, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair falling past her "
           "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
           "eyes and soft coral-pink lipstick. She wears a swimsuit: a plain white bikini top with thin white "
           "shoulder straps and a small knot at the centre front, her shoulders and arms bare. A large bright "
           "red hibiscus flower with a long yellow-tipped stamen is pinned at her left shoulder strap. No "
           "jewellery is visible."},
  {5, 154, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair here gathered into a high ponytail at the back of her head that spills in big loose "
           "curls, with her long fringe still over her left eye and loose strands framing her cheeks. She is "
           "wrapped in a dusty pink terry-cloth bath towel tucked straight across her chest, its fluffy edge "
           "just above her bust; her shoulders, collarbones and arms are bare. The portrait ends at the top of "
           "the towel."},
  {5, 155, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair here gathered into a high ponytail at the back of her head that spills in big loose "
           "curls, with her long fringe still over her left eye. She wears an inn's yukata: a white cotton robe "
           "crossed at the neck, printed with a chain of grey outlined rings joined by small dark grey dots, "
           "with thin grey lines along the collar. Over it hangs a loose, wide-sleeved slate-blue haori jacket, "
           "worn open."},
  {5, 156, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair here gathered into a high ponytail at the back of her head that spills in big loose "
           "curls, with her long fringe still over her left eye. She wears a summer festival yukata of white "
           "cotton with thin pink stripes in pairs, printed with sprays of slender green leaves and hanging "
           "clusters of purple and pink bell-shaped flowers with yellow centres, with loose pink petals falling "
           "between them. A green sash shows at her back."},
  {5, 157, "Mitsuru, a tall young woman with red eyes and crimson hair, pinned up and decorated with a large "
           "white silk flower, red camellias, gold cords and a hanging gold tassel. She wears a formal New Year "
           "kimono: dark green silk with large red camellia blossoms and scattered orange dots, red and gold "
           "patterned collars, and a gold-patterned sash."},
  {5, 159, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair here gathered into a high ponytail at the back of her head that spills in big loose "
           "curls, with her long fringe still over her left eye. She wears a long-sleeved white turtleneck "
           "sweater in a fine vertical rib knit, with a tall close neck. Over it is a dark brown bib apron hung "
           "from olive-khaki straps with dashed stitching along their edges."},
  {6, 1, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
         "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. In her school "
         "summer uniform: a white long-sleeved blouse of smooth fabric with a wide, round neckline edged in a "
         "scalloped lace trim, tied at the front with a thin white ribbon bow, over a fitted bright teal "
         "turtleneck that covers her neck. No jewellery is visible."},
  {6, 2, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in her school winter "
         "uniform. A dark charcoal blazer with silver piping, a strap with two round silver buttons across the "
         "chest and a red school crest sits over a high teal turtleneck with a faint lace pattern. A thin "
         "white ribbon is tied at the back of her neck, its ends trailing over one shoulder."},
  {6, 5, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
         "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. In summer "
         "casual clothes: a sleeveless mint-teal dress with wide ruffled shoulder straps and a square neckline "
         "edged with a soft ruffle and outlined by a thin gold line, leaving her arms and collarbones bare. No "
         "jewellery is visible."},
  {6, 6, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in winter casual "
         "clothes. A soft green poncho with white embroidered flower motifs has a wide cream collar trimmed "
         "with fur, tied at the front with a white ribbon ending in fluffy pompoms. Underneath is a grey "
         "ribbed turtleneck sweater."},
  {6, 52, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in her support gear "
          "for the Dark Hour. She wears a heavy charcoal greatcoat with a tall stand-up collar fastened by a "
          "strap, brass cross-marked buttons and a short shoulder cape, over a teal top. A silver headset with "
          "large ear cups and a thin microphone arm sits over her hair, and she wears dark fingerless gloves."},
  {6, 102, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
           "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. In a swimsuit: "
           "a plain mint-teal top cut straight across above her bust, with no print or trim, held by thin white "
           "straps that run up to a halter tie behind her neck, its bow just visible. Her shoulders, collarbones "
           "and arms are bare, and no jewellery is visible."},
  {6, 154, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
           "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. She is wrapped "
           "in a dusty pink terry-cloth bath towel tucked straight across her chest, its fluffy edge just above "
           "her bust, as at a hot spring; her shoulders, collarbones and arms are bare. The portrait ends at the "
           "top of the towel."},
  {6, 155, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
           "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. In an inn's "
           "robes: a white cotton yukata crossed at the neck, printed with a chain of grey outlined rings joined "
           "by small dark grey dots, with thin grey lines along the collar. Over it hangs a loose, wide-sleeved "
           "slate-blue haori jacket, worn open, of plain matte cloth."},
  {6, 156, "Fuuka, a slight, pale-skinned teenage girl with a short teal bob that falls in feathered layers to "
           "her jaw, straight-cut bangs and large grey eyes, wearing no jewellery. She wears a summer festival "
           "yukata of mid-blue cotton, crossed left over right at a V neckline that shows a sliver of white "
           "underlayer. It is printed with large sunflowers, golden-yellow petals around brown centres dotted "
           "with yellow seeds, scattered over faint pale green rings. Behind her back at the waist an orange obi "
           "bow just shows; the portrait ends at her upper chest."},
  {6, 157, "Fuuka, a slight girl with a short teal bob, straight-cut bangs and grey eyes, in a New Year's "
           "kimono. The kimono is bright teal with white, yellow and red chrysanthemums, its collar layered in "
           "cream, orange and olive, with a pink obi in a red diamond pattern and a yellow bow at the back. "
           "Above one ear she wears an ornament of an orange and a yellow fabric flower with green cords ending "
           "in small silver beads."},
  {6, 901, "Fuuka, shown only as a flat dark grey silhouette, her features shaded over by a darker band so only "
           "faint outlines of eyes and mouth remain. The outline shows her short, feathered bob, the tall "
           "stand-up collar of her school winter uniform closed at the throat, the shoulders of the blazer with "
           "a faint button or two, and a thin ribbon tied at the back of her neck whose loops and ends trail "
           "down behind one shoulder. No colour or pattern can be made out."},
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
  {8, 1, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its bangs swept to "
         "one side and its ends flicking outward over his ears, and large brown eyes. He wears a sleeveless "
         "orange hooded vest, unzipped, with a silver zip edge and its hood bunched behind his neck. Under it "
         "is a white long-sleeved collared shirt with small white buttons, and at the collar a thin black "
         "string tie knotted in a loose bow whose two long ends hang down his chest. No jewellery."},
  {8, 2, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
         "outward over his ears, and large brown eyes. He wears his school winter uniform: an open dark grey "
         "zip jacket with a silver zip, a diagonal silver zip pocket and a small brass snap on the chest, and "
         "a round badge with a red ring lettered in gold Gekkoukan Private Elementary School around a "
         "black-and-white quartered emblem. Under it are an orange hoodie, its hood spread behind his neck, "
         "and a white collared shirt with a thin black string tie in loose ends."},
  {8, 5, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
         "outward over his ears, and large brown eyes. He wears casual clothes: a zip-up track jacket with a "
         "silver zip, its front and sleeve panels burnt orange and its shoulder yoke and upper sleeves mustard "
         "yellow, every seam edged with white piping. Under it a dark grey crew-neck T-shirt shows at his "
         "throat. No jewellery or accessories."},
  {8, 6, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
         "outward over his ears, and large brown eyes. He wears winter casual clothes: a thick, chunky knit "
         "sweater in cream with a tall, loose rolled turtleneck that hides his neck up to the chin, its front "
         "worked in vertical ribs and cable stitches. The shoulders are tan and the sleeves a deeper caramel "
         "brown, sewn on in raglan panels. No jewellery or accessories."},
  {8, 52, "Ken, a small boy with a soft, tousled brown bob and large brown eyes. He wears his school outfit "
          "ready for battle: a dark grey zip jacket over an orange hoodie and a white collared shirt with a "
          "thin black string tie, a black strap over his shoulder, and a red armband with a black-and-white "
          "emblem around his upper arm."},
  {8, 159, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
           "outward over his ears, and large brown eyes. He wears a plain lemon-yellow T-shirt with a dark grey "
           "ribbed crew neckline and short sleeves. Over it is a rust-brown apron hung from narrow straps over "
           "each shoulder, its edges and straps lined with dashed white stitching and a stitched pocket across "
           "the bib, as if he is cooking. The portrait ends at his upper chest."},
  {9, 6, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears with grey insides, two grey brow "
         "spots, a black nose, a white face and chest, and soft grey shading over his neck, back and thick "
         "curled tail. He wears a sleeveless cream-beige canvas vest with dashed stitching along its seams. "
         "Around his neck is a thick grey metal collar with a round housing on one side capped by a glowing "
         "blue glass dome, and behind it a small white wing on a white disc held by a slotted pale green screw "
         "cap, sticking out like an angel wing."},
  {9, 52, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears, two grey brow spots and a thick "
          "curled tail. In his battle gear he wears the tan vest and grey mechanical collar with a glowing blue "
          "dome and small white wing, plus a black harness with brass-buckled straps, a round brass tag hanging "
          "at his chest, a white canister with red stripes on his back and a red armband on his foreleg."},
  {9, 201, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears with grey insides, two grey brow "
           "spots, a black nose and a white face. Grey shading runs over his neck, shoulders and back, his chest "
           "is a fluffy paler white ruff, and his thick tail curls up behind him. He wears nothing at all: none "
           "of his usual tan vest or his metal collar with the blue dome and white wing, just his thick fur."},
  {10, 6, "Shinjiro, a tall young man with shoulder-length, messy brown hair falling over grey eyes, and a long "
          "jaw. He wears a dark grey knit beanie with a small metal plate on the front and a long maroon-red "
          "double-breasted pea coat, buttoned up, with wide dark grey lapels and collar studded with small "
          "silver rivets."},
  {10, 52, "Shinjiro, a tall young man with shoulder-length, messy brown hair falling over grey eyes, and a long "
           "jaw. He wears a dark grey knit beanie with a small metal plate on the front and his long maroon-red "
           "coat with wide studded dark grey lapels, worn open over a black high-necked sweater, a belt buckle "
           "showing at the bottom."},
  {10, 159, "Shinjiro, a tall young man with fair skin, a long jaw and grey eyes, his shoulder-length brown hair "
            "messy and falling over his face and out behind his neck in ragged strands. He wears his dark grey "
            "knit beanie pulled low, its folded ribbed brim fronted by a small rectangular metal plate with two "
            "rivets and a slanted line engraved on it. Below it is a plain charcoal long-sleeved top with a high "
            "mock neck. Over it hangs a tan canvas apron on narrow straps, with dashed stitching along its edges "
            "and a stitched bib pocket, as if he is cooking."},
  {11, 52, "Metis, a slender girl with a short blue-black bob and red eyes. On her head sits a large gold-framed "
           "crown of red butterfly-wing panels with round red ear discs pierced by gold spikes, and it folds "
           "down into a wing-shaped visor over her face. Her body is a seamless dark grey suit with white "
           "jointed shoulder parts, and a red armband with a black-and-white emblem is fixed to her upper arm."},
  {11, 201, "Metis, a slender girl with a short blue-black bob and red eyes. On her head sits a large gold-framed "
            "crown of red butterfly-wing panels with round red ear discs pierced by gold spikes, and it folds "
            "down into a wing-shaped visor over her face. Her body is a seamless dark grey suit with white "
            "jointed shoulder parts; unlike her battle look she wears no armband."},
  {12, 2, "Aigis, a girl-shaped android with fair, smooth skin, short blond hair in a choppy bob with long "
          "side-swept bangs, and bright blue eyes. A gold headband of open triangular lattice sits on her head, "
          "and round white mechanical ear units with red three-lobed centres cover her ears; her neck is white "
          "and jointed. She wears the Gekkoukan winter uniform: a black blazer edged with grey piping, a white "
          "collar at her throat, a large red ribbon bow with ruffled tails, a small silver zip pull, and a "
          "round red-ringed badge reading Gekkoukan Private High School around a black-and-white quartered "
          "emblem."},
  {12, 6, "Aigis, a girl-shaped android with fair, smooth skin, short blond hair in a choppy bob with long "
          "side-swept bangs, and bright blue eyes. A gold headband of open triangular lattice sits on her head, "
          "round white mechanical ear units with red three-lobed centres cover her ears, and her white jointed "
          "neck shows above her collar. She wears casual clothes: a white blouse with a high frilled collar and "
          "a pleated front, a large, flowing white cravat tied in a bow with long tails down her chest, and "
          "over it an open teal jacket with wide lapels."},
  {12, 52, "Aigis, a girl-shaped android with short blond hair, blue eyes, a gold lattice headband, "
           "white-and-red mechanical ear units and a white jointed neck. Her robot body shows: a white chest "
           "with a large red bow at the throat, gold mechanical shoulder joints, and a red armband with a "
           "black-and-white emblem on her upper arm."},
  {12, 53, "Aigis, a girl-shaped android with fair, smooth skin, short blond hair in a choppy bob with long "
           "side-swept bangs, and bright blue eyes. A gold headband of open triangular lattice sits on her head, "
           "round white mechanical ear units with red three-lobed centres cover her ears, and her neck is white "
           "and jointed. Her robot body shows bare: a smooth white chest with a large red ribbon bow with "
           "ruffled tails at the throat, and bulky gold shoulder joints with black ribbed cylinders, round black "
           "caps and open lattice plates. She wears no armband."},
  {12, 54, "Aigis, a girl-shaped android with short blond hair, blue eyes, white-and-red mechanical ear units "
           "and a white jointed neck. She wears armour: a steel visor band edged in gold across her forehead "
           "over the lattice headband, a steel gorget hanging on chains over her red bow, a dark harness lined "
           "with brass bullet cartridges, ribbed gold shoulder armour, and a red armband with a black-and-white "
           "emblem."},
  {12, 201, "Aigis, a girl-shaped android with short blond hair, blue eyes, white-and-red mechanical ear units "
            "and a white jointed neck. She wears armour: a steel visor band edged in gold across her forehead, a "
            "steel gorget on chains over her red bow, a dark harness lined with brass bullet cartridges and "
            "ribbed gold shoulder armour, with no armband."},
  {101, 1, "Kenji, a slim teenage boy with fair skin, short, messy light-brown hair falling in uneven bangs over "
           "his forehead and ears, and brown eyes. He wears the school summer uniform: a short-sleeved white "
           "shirt with a pointed collar, small white buttons and a breast pocket. A thin black ribbon tie is "
           "knotted loosely at the collar, its two long ends hanging crookedly down his chest. No jewellery or "
           "accessories."},
  {101, 2, "Kenji, a slim teenage boy with fair skin, short, messy light-brown hair falling in uneven bangs over "
           "his forehead and ears, and brown eyes. He wears the school winter uniform: a black blazer with "
           "notched lapels and a silver strip running diagonally across the chest, over a white collared shirt "
           "with a thin black ribbon tie hanging loose. On the lapel is a small rectangular pin, half green and "
           "half yellow, and on the chest the round red-ringed badge reading Gekkoukan Private High School "
           "around a black-and-white quartered emblem."},
  {101, 6, "Kenji, a slim teenage boy with fair skin, short, messy light-brown hair falling in uneven bangs over "
           "his forehead and ears, and brown eyes. In casual clothes he wears an open bright mustard-yellow zip "
           "jacket with a silver zip, a high stand-up collar lined in white, and a row of small snap buttons "
           "down the front. Under it is a dark grey V-neck T-shirt layered over a white crew-neck one whose "
           "collar shows at his throat. No jewellery or accessories."},
  {101, 158, "Kenji, a slim teenage boy with fair skin, short light-brown hair poking up over a cap band and "
             "falling over his ears, and brown eyes. He wears a fast-food work uniform: an orange visor with a "
             "long pale yellow brim and a large looping cursive W logo in pale yellow on its front band. His "
             "short-sleeved shirt is orange with a pale yellow pointed collar, a pale yellow button placket with "
             "round buttons, a pale yellow strip across the breast pocket, and pale bands at the sleeve ends."},
  {102, 1, "Hidetoshi, a lean teenage boy with fair skin, dark grey hair slicked back from a high forehead, a "
           "few strands curling forward over it and the ends flicking out at his nape, and narrow grey eyes. He "
           "wears the school summer uniform: a short-sleeved white shirt with a pointed collar, small buttons "
           "and a breast pocket, with a thin black ribbon tie knotted at the collar and its ends hanging down. "
           "On his left upper arm is a mustard-yellow armband with large black kanji characters, cut off by the "
           "picture's edge."},
  {102, 2, "Hidetoshi, a lean teenage boy with dark grey hair slicked back from a high forehead and narrow grey "
           "eyes. He wears the school winter uniform, a black blazer with the school crest and a small lapel pin "
           "over a white shirt and thin black ribbon tie, with a yellow armband with black lettering over the "
           "blazer sleeve."},
  {103, 6, "Bunkichi, an elderly man with wild, thinning white hair, bushy white eyebrows, a white moustache and "
           "black rectangular glasses, with a dark mark on his bald forehead. He wears a cream knitted sweater "
           "with a dark ribbed turtleneck collar and a red and blue zigzag pattern across the chest and arms, "
           "and holds a dark wooden tobacco pipe."},
  {104, 6, "Mitsuko, a small, very old woman with pale skin, a heavily wrinkled round face with sagging cheeks "
           "and pouched eyes, and thinning grey hair parted in the middle and cut to chin length. She wears a "
           "lilac blouse with a round collar and small buttons, printed all over with grey-blue leafy sprigs and "
           "tiny cream flowers. Over it is a dark olive-green work apron held by broad shoulder straps, each "
           "fastened to the bib with a round brass button. No jewellery."},
  {105, 101, "Kazushi, an athletic teenage boy with lightly tanned skin, short, spiky dark grey hair standing up "
             "on top and cut close at the sides, and dark eyes. He wears a sporty track jacket in grey zipped up "
             "to a tall stand-up collar, with a silver zip pull at his throat. Twin white stripes run down either "
             "side of the zip and over the tops of his shoulders, and red panels sit low on each side of the "
             "chest. No jewellery or accessories."},
  {105, 151, "Kazushi, an athletic teenage boy with lightly tanned skin, short, spiky dark grey hair standing up "
             "on top and cut close at the sides, and dark eyes. He wears a dark grey sleeveless athletic vest, a "
             "basketball-style tank top with a deep scooped neck, its neckline and armholes edged in red trim with "
             "a black-and-red patch at one shoulder. His lean shoulders, collarbones and arms are bare. No "
             "jewellery or accessories."},
  {106, 1, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
           "ponytail sticking up at the back of her head, a slim pink hairpin clipped above her left temple, and "
           "brown eyes. She wears the school summer uniform: a short-sleeved white blouse with a stand-up "
           "collar, small white buttons down the front and a breast pocket, and a large red ribbon bow at the "
           "collar with ruffled tails. No earrings or other jewellery."},
  {106, 2, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
           "ponytail sticking up at the back, a slim pink hairpin above her left temple, and brown eyes. She "
           "wears the school winter uniform: a black jacket edged with grey piping, with a front flap fastened "
           "by silver buttons and a small silver zip pull, over a white collared blouse with a large red ribbon "
           "bow. On the chest is the round red-ringed badge reading Gekkoukan Private High School around a "
           "black-and-white quartered emblem."},
  {106, 6, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
           "ponytail sticking up at the back, a slim pink hairpin above her left temple, and brown eyes. In "
           "casual clothes she wears an open black zip-up hoodie with a silver zip, its hood lined in white and "
           "folded around her neck, and a white stripe running down each sleeve from the shoulder. Under it is a "
           "coral-red top with a wide scooped neck that shows her collarbones. No jewellery."},
  {106, 101, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
             "ponytail sticking up at the back, a slim pink hairpin above her left temple, and brown eyes. She "
             "wears a sporty grey track jacket zipped up to a tall stand-up collar with a silver zip pull at her "
             "chin. Twin white stripes run down either side of the zip and over the tops of her shoulders, and red "
             "panels sit low on each side of the chest. No jewellery."},
  {107, 6, "Maya, an online game avatar drawn as a translucent green hologram made of flat, low-polygon shapes "
           "with horizontal scan lines and a pink and cyan glow at the edges. The figure has a short, "
           "flicked-out bob of hair, one visible eye, and a bulky high-collared coat with stripes and diamond "
           "shapes across it."},
  {108, 1, "Keisuke, a slim teenage boy with fair skin and thick, soft, wavy light-brown hair that falls in "
           "loose curls over his forehead, ears and the back of his neck. He has grey eyes behind thin black "
           "rectangular glasses. He wears the school summer uniform: a crisp white collared shirt with small "
           "white buttons and a breast pocket, and a thin dark green-black ribbon tied loosely at the collar, "
           "its two long ends hanging down his chest. The portrait ends at mid-chest."},
  {108, 2, "Keisuke, a slim teenage boy with fair skin and thick, soft, wavy light-brown hair that falls in "
           "loose curls over his forehead, ears and the back of his neck. He has grey eyes behind thin black "
           "rectangular glasses. He wears the school winter uniform: a charcoal-black blazer with notched lapels "
           "and a thin silver-white piping down its front edge, a small green-and-cream bar pin on the left "
           "lapel, and the school crest on the chest, a red ring lettered Gekkoukan Private High School around a "
           "black-and-white quartered circle. Under it are a white collared shirt and a thin dark green-black "
           "ribbon tie with long loose ends."},
  {108, 6, "Keisuke, a slim teenage boy with fair skin and thick, soft, wavy light-brown hair that falls in "
           "loose curls over his forehead, ears and the back of his neck. He has grey eyes behind thin black "
           "rectangular glasses. In casual clothes he wears a collared button-up shirt in a bold plaid of red, "
           "sky blue, white and grey checks, with small grey buttons, open at the neck over a dark green T-shirt "
           "whose round collar just shows. The portrait ends at mid-chest."},
  {109, 1, "Chihiro, a slender teenage girl with fair skin and very long, straight brown hair parted in the "
           "middle and swept back off a high forehead, falling past her shoulders and down her back with long "
           "strands framing her face. She has brown eyes behind black rectangular glasses and soft pink lips. "
           "She wears the school summer uniform: a white collared blouse with small white buttons, and a large, "
           "full red bow tied at the collar, its loops and ends spreading over her chest."},
  {109, 2, "Chihiro, a slender teenage girl with fair skin and very long, straight brown hair parted in the "
           "middle and swept back off a high forehead, falling past her shoulders and down her back with long "
           "strands framing her face. She has brown eyes behind black rectangular glasses and soft pink lips. "
           "She wears the school winter uniform: a charcoal-black jacket with a thin silver-white piping down "
           "its front edge and the red school crest just visible on the chest, over a white collared blouse with "
           "white cuffs and a large, full red bow tied at the collar."},
  {109, 6, "Chihiro, a slender teenage girl with fair skin and very long, straight brown hair parted in the "
           "middle and swept back off a high forehead, falling past her shoulders and down her back with long "
           "strands framing her face. She has brown eyes behind black rectangular glasses and soft pink lips. In "
           "casual clothes she wears a soft, warm tan-orange cardigan with a frilled ruffle running down its "
           "button front, over a cream top with a high, close-fitting collar whose upper edge is cut in a "
           "zigzag."},
  {110, 6, "Maiko, a little girl of about eight with fair skin, a round face and big brown eyes. Her brown hair "
           "is pulled smoothly back off her forehead into a braided bun at the back of her head, wrapped with a "
           "ruffled pink scrunchie. She wears an open, slightly oversized denim-blue shirt with a pointed "
           "collar, silver snap buttons and rolled sleeves over a plain pink T-shirt with a round neck. On her "
           "back is a glossy red leather school backpack, a randoseru, with red shoulder straps, silver metal "
           "fittings and a white cord dangling from a clip at its side."},
  {111, 6, "Pharos, a small, slight, pale boy who looks about ten, with messy, tousled dark blue-grey hair cut "
           "short with ragged bangs over his forehead, and large, bright cyan-blue eyes. He has a small dark "
           "mole under his left eye. He wears a long-sleeved shirt with wide horizontal stripes of dark grey and "
           "off-white, a plain round neckline, and a short placket at the front fastened with three small grey "
           "buttons."},
  {112, 1, "Bebe, a slim teenage boy with light skin and a neat, rounded blond bowl cut with thick straight "
           "bangs down to his eyebrows, and pale turquoise-blue eyes. He wears the school summer uniform: a "
           "white collared shirt with short sleeves, small white buttons and a breast pocket, and a thin black "
           "ribbon tie with long loose ends. He holds up an open Japanese folding fan of black paper on black "
           "ribs, painted with two gold butterflies above sweeping gold arcs and scattered gold dots."},
  {112, 2, "Bebe, a slim teenage boy with light skin and a neat, rounded blond bowl cut with thick straight "
           "bangs down to his eyebrows, and pale turquoise-blue eyes. He wears the school winter uniform: a "
           "charcoal-black blazer with thin silver-white piping down its front edge, a small green-and-cream bar "
           "pin on the lapel and the school crest on the chest, a red ring lettered Gekkoukan Private High "
           "School around a black-and-white quartered circle, over a white collared shirt and a thin black "
           "ribbon tie. He holds up an open black folding fan painted with two gold butterflies above sweeping "
           "gold arcs and scattered gold dots."},
  {112, 6, "Bebe, a slim teenage boy with light skin and a neat, rounded blond bowl cut with thick straight "
           "bangs down to his eyebrows, and pale turquoise-blue eyes. In casual clothes he wears a plain dark "
           "charcoal-grey long-sleeved T-shirt with a round crew neck, printed on the chest with a faint, "
           "tone-on-tone grey Eastern dragon, its scaly body coiled and its horned head turned to one side. The "
           "portrait ends at mid-chest."},
  {113, 6, "President Tanaka, a middle-aged man with tanned skin, a long face with lines under his eyes, and "
           "thick, wavy, collar-length dark grey hair swept back and falling loosely over his ears. He has "
           "narrow, heavy-lidded grey eyes. He wears a grey-green suit jacket with notched lapels over a crisp "
           "white dress shirt with a pointed collar and a plain dark red tie in a neat knot."},
  {114, 6, "Mutatsu, an elderly, heavyset and completely bald man with tanned, deeply lined skin, a furrowed "
           "brow, large ears, grey eyes and loose skin at his neck. He wears a Buddhist monk's robes: a dark "
           "charcoal-grey kimono-style robe crossed at the chest over white under-layers whose collars show at "
           "the neck, with a broad pale cream band running down over one shoulder and tied in a knot low on his "
           "chest. In one hand he holds a fat brown cigar, its ashy tip lit, and thick gold rings on his "
           "fingers."},
  {114, 901, "Mutatsu, an elderly, heavyset and completely bald man with tanned, deeply lined skin, a furrowed "
             "brow, large ears, grey eyes and loose skin at his neck. He wears a Buddhist monk's robes: a dark "
             "charcoal-grey kimono-style robe crossed at the chest over white under-layers whose collars show at "
             "the neck, with a broad pale cream band running down over one shoulder and tied in a knot low on his "
             "chest. Unlike his usual look, he holds no cigar and no hand or ring is in view."},
  {115, 6, "Mamoru, an athletic teenage boy with lightly tanned skin and short, spiky dark grey hair cropped "
           "close at the sides, with thick dark eyebrows and grey eyes. He wears a light teal zip-up hoodie with "
           "the hood down, a silver zipper, grey drawstrings knotted at their ends, and a small dark grey "
           "triangle logo on the chest. Down each sleeve runs a wide black stripe edged in white and printed "
           "with a row of grey triangles."},
  {116, 1, "Nozomi, a heavyset teenage boy with fair skin, a big round face with full, chubby cheeks and a "
           "double chin, short neat black hair with one curl springing up on top and a curl on his forehead, "
           "droopy grey eyes and large ears. He wears the school summer uniform: a white collared shirt "
           "stretched over his broad frame, with small white buttons, a breast pocket and a thin dark "
           "green-black ribbon tie with long loose ends."},
  {116, 2, "Nozomi, a heavyset teenage boy with fair skin, a big round face with full, chubby cheeks and a "
           "double chin, short neat black hair with one curl springing up on top and a curl on his forehead, "
           "droopy grey eyes and large ears. He wears the school winter uniform: a charcoal-black blazer with "
           "thin silver-white piping down its front edge, a small green-and-cream bar pin on the lapel and the "
           "school crest on the chest, a red ring lettered around a black-and-white quartered circle, over a "
           "white collared shirt and a thin dark green-black ribbon tie with long loose ends."},
  {117, 6, "Akinari, a thin, pale young man with a narrow face, straight chin-length silver-grey hair with a "
           "side parting and bangs sweeping across his forehead, and grey eyes. He wears a loose, long-sleeved "
           "button-up shirt with a pointed collar, in bold vertical stripes of slate grey-blue and pale "
           "grey-white, with small dark buttons, the top ones undone so it falls open at the neck and shows his "
           "collarbones."},
  {201, 6, "Igor, a short, hunched, wizened old man with pale tan skin and a bald dome of a head with wild, "
           "wispy white hair sticking out at the back and sides. He has long pointed ears, thick dark curling "
           "eyebrows, huge round bulging white eyes threaded with red veins and tiny pupils, and an extremely "
           "long, straight pointed nose. He wears a black suit jacket with buttoned cuffs over a white shirt and "
           "dark tie, white shirt cuffs and thin white gloves on his long bony fingers."},
  {202, 6, "Elizabeth, a slender young woman with pale skin, a short silver bob with straight blunt bangs and "
           "the ends curving in at her jaw, bright yellow eyes and pale pink lips. She wears her Velvet Room "
           "attendant uniform: a small royal-blue cap with a short brim and a button on its side, tilted on her "
           "head, and a fitted royal-blue sleeveless dress that leaves her shoulders bare, with a tall stand-up "
           "collar and a dark seam down the front. On the chest is a large round emblem: a dark navy ring, a "
           "sky-blue ring and a dark centre with a small gold dot. Long royal-blue gloves reach above her "
           "elbows."},
  {202, 901, "Elizabeth, a slender young woman with pale skin, a short silver bob with straight blunt bangs, "
             "bright yellow eyes and pale pink lips, wearing a pair of chunky black glasses with thick, "
             "rounded-square frames. She wears her Velvet Room attendant uniform: a small royal-blue cap with a "
             "short brim and a side button, tilted on her head, and a fitted royal-blue sleeveless dress with bare "
             "shoulders, a tall stand-up collar and a large round emblem on the chest, dark navy and sky-blue "
             "rings around a dark centre with a gold dot. Long royal-blue gloves reach above her elbows."},
  {202, 902, "Elizabeth, a slender young woman with pale skin, a short silver bob with straight blunt bangs and "
             "bright yellow eyes, dressed up as a cat: a pair of fluffy white cat ears with pink insides stands up "
             "on either side of her head, and a white furry cat-muzzle mask with a pink nose and black whiskers "
             "covers her nose and upper lip. She still wears her small tilted royal-blue cap and her Velvet Room "
             "attendant uniform: a fitted royal-blue sleeveless dress with bare shoulders, a tall stand-up collar "
             "and a large round emblem on the chest, dark navy and sky-blue rings around a dark centre with a gold "
             "dot. Long royal-blue gloves reach above her elbows."},
  {202, 903, "Elizabeth, a slender young woman with pale skin and a short silver bob with straight blunt bangs, "
             "wearing joke novelty glasses: round black frames with white lenses painted with swirling circles "
             "that hide her eyes, a big pinkish-red plastic nose and a bushy black moustache curled up at the "
             "ends. She still wears her small tilted royal-blue cap and her Velvet Room attendant uniform: a "
             "fitted royal-blue sleeveless dress with bare shoulders, a tall stand-up collar and a large round "
             "emblem on the chest, dark navy and sky-blue rings around a dark centre with a gold dot. Long "
             "royal-blue gloves reach above her elbows."},
  {202, 904, "Elizabeth, a slender young woman with pale skin, a short silver bob with straight blunt bangs and "
             "bright yellow eyes, dressed as Santa: in place of her cap she wears a red Santa hat with a thick "
             "white fur brim, its tip flopping over to one side and ending in a fluffy white pom-pom, and a fluffy "
             "white false beard and curled moustache that cover her mouth and chin. Below it is her Velvet Room "
             "attendant uniform: a fitted royal-blue sleeveless dress with bare shoulders, a tall stand-up collar "
             "and a large round emblem on the chest, dark navy and sky-blue rings around a dark centre with a gold "
             "dot. Long royal-blue gloves reach above her elbows."},
  {211, 6, "Takaya, a gaunt, lean man with pale skin, sunken cheeks and stubble on his jaw and chin, and yellow "
           "eyes. His very long, wavy silver-grey hair is parted in the middle and falls in loose waves past his "
           "shoulders and down his chest, held by a thin dark metal circlet across his forehead. He is "
           "bare-chested, with purple tattoos on both shoulders: on his right, a crowned serpent coiled around a "
           "spiky sun with a pink spiral at its centre; on his left, a tangle of purple and pale figures mostly "
           "hidden by his hair. The portrait ends at mid-chest."},
  {212, 6, "Jin, a lean young man with pale skin and straight, glossy dark blue hair cut in a jaw-length bob, "
           "its long fringe falling over his left eye. He wears glasses with black rectangular frames and "
           "orange-tinted lenses. His bright green jacket is zipped right up into a tall stand collar that hides "
           "his chin, with two light grey stripes running down the front beside the zip. Black harness straps "
           "with buckles run over both shoulders, and a black band is wrapped around his left upper arm. The "
           "portrait ends at his upper chest."},
  {213, 6, "Chidori, a pale young woman with very long, straight red hair with blunt bangs and grey eyes with "
           "dark lashes. She wears a frilly white gothic-lolita dress with ruffles, a ribbon choker and ribbons "
           "laced through her hair, a black and white ruffled headdress studded with silver diamonds, and a "
           "large silver dagger-shaped pin through it."},
  {213, 201, "Chidori, a slender, pale young woman with very long, straight red hair with blunt bangs cut straight "
             "across her brows, the rest falling loose past her shoulders and down her back. She has grey eyes "
             "rimmed with spiky dark lashes, soft purple eyeshadow and pale pink lips. She is without her usual "
             "headdress and ribbons and wears no jewellery. She wears a plain pale aqua-blue hospital gown with a "
             "wrap-over front crossing at her chest and loose sleeves, without any pattern or trim. The portrait "
             "ends at her chest."},
  {221, 2, "Ryoji, a slender teenage boy with fair skin and dark grey-black hair swept back from his forehead, a "
           "few strands sticking up on top and the ends flicking out at his nape. He has blue eyes and a small "
           "black beauty mark under his left eye. He wears a white button-front shirt with small dark square "
           "buttons, with narrow dark bands running down either side of his chest, the edges of his open school "
           "winter jacket. A long, soft pale-yellow scarf, edged in a darker outline, is wrapped high around his "
           "neck and hangs down the front. The portrait ends at his chest."},
  {221, 154, "Ryoji, a slender teenage boy with fair skin and dark grey-black hair swept back from his forehead, a "
             "few strands sticking up on top and the ends flicking out at his nape. He has blue eyes and a small "
             "black beauty mark under his left eye. He is bare-chested and bare-shouldered, as at a bath or hot "
             "spring, with no clothing or jewellery at all, not even his usual yellow scarf. The portrait ends at "
             "his upper chest."},
  {221, 155, "Ryoji, a slender teenage boy with fair skin and dark grey-black hair swept back from his forehead, a "
             "few strands sticking up on top and the ends flicking out at his nape. He has blue eyes and a small "
             "black beauty mark under his left eye. He wears an inn-style yukata: a plain slate grey-blue outer "
             "robe with wide sleeves, wrapped over a white inner robe whose edged collar shows in a V at his "
             "throat, patterned with grey chains of interlinked circles. He wears no scarf or jewellery. The "
             "portrait ends at his chest."},
  {222, 6, "Ikutsuki, a lanky middle-aged man with pale skin and long, wavy brown hair combed back from a high, "
           "receding hairline and falling loose behind his shoulders. He has grey eyes behind thin silver "
           "wire-rimmed glasses with narrow oval lenses, and a small pointed goatee. He wears a light tan "
           "double-breasted coat with wide notched lapels and pale buttons, over a dark grey turtleneck sweater "
           "knitted in vertical ribs, its high collar folded up under his chin. The portrait ends at his chest."},
  {223, 1, "Natsuki, a teenage girl with tanned skin and long, wavy ginger-brown hair with straight, blunt "
           "bangs, part of it gathered into a short, spiky ponytail tied high at the back of her head and the "
           "rest falling past her shoulders. She has brown eyes with thick lashes and glossy lips. She wears the "
           "school summer uniform: a white short-sleeved blouse with a wide, pointed collar, a dark grey knitted "
           "V-neck sweater vest with ribbed edges, and a large red bow tied at the collar. The portrait ends at "
           "her chest."},
  {223, 2, "Natsuki, a teenage girl with tanned skin and long, wavy ginger-brown hair with straight, blunt "
           "bangs, part of it gathered into a short, spiky ponytail tied high at the back of her head and the "
           "rest falling past her shoulders. She has brown eyes with thick lashes and glossy lips. She wears the "
           "school winter uniform: a dark grey knitted V-neck cardigan with long sleeves and ribbed edges over a "
           "white blouse with a wide, pointed collar, and a large red bow tied at the collar. The portrait ends "
           "at her chest."},
  {223, 6, "Natsuki, a teenage girl with tanned skin and ginger-brown hair with straight, blunt bangs and a "
           "short, spiky ponytail tied high at the back, the rest hanging just to her shoulders. She has brown "
           "eyes with thick lashes and glossy lips. In casual clothes, she wears an oversized cream hooded "
           "jacket, hanging open with its hood bunched behind her neck, its lining grey-blue and a round snap "
           "button on each edge of the front. Under it is a pink scoop-neck top, and a small silver heart-shaped "
           "pendant hangs on a thin chain at her collarbone. The portrait ends at her chest."},
  {224, 6, "Takeharu, a stern middle-aged man with pale skin and short dark grey hair swept across his forehead "
           "to one side. A black eyepatch covers his right eye, its thin black strap running diagonally up "
           "across his forehead and into his hair; his other eye is grey. He wears a grey three-piece suit: a "
           "jacket with notched lapels, a darker charcoal waistcoat, a white shirt with a pointed collar and a "
           "dark red tie. A small round gold pin is fixed on his left lapel. The portrait ends at his chest."},
  {252, 1, "Akihiko as a young boy, with fair skin and short, tousled silver-white hair, grey eyes and a beige "
           "sticking plaster across the bridge of his nose. He wears a white collared button-up shirt, "
           "unbuttoned at the neck, with a breast pocket on the left, over a plain red T-shirt whose round neck "
           "shows at his throat. He wears no jewellery. The portrait ends at his chest."},
  {253, 1, "Mitsuru as a younger teenager, with fair skin and wavy red hair falling in loose curls past her "
           "shoulders, a long side-swept fringe and a thin purple hairband. She has red eyes and soft pink lips. "
           "She wears a school uniform: a white long-sleeved blouse with a small pointed collar, a dark grey "
           "knitted sweater vest with ribbed edges, and a large red bow at the collar with long, layered ruffled "
           "tails. She wears no jewellery. The portrait ends at her chest."},
  {254, 5, "Mitsuru as a little girl, with fair skin, red eyes and red hair with a side-swept fringe, the rest "
           "worn in long, springy ringlet drill curls hanging on both sides of her head. She wears a white "
           "long-sleeved blouse with fine vertical pleats down the front, small round white buttons and a high "
           "stand collar. A large red bow is tied at the collar, fastened at its centre by a gold-framed oval "
           "brooch set with a dark green-black stone. The portrait ends at her chest."},
  {1081, 6, "Mr. Edogawa, a lanky man with pale skin, messy, spiky dark hair, thick black rectangular glasses and "
            "heavy stubble on his chin and jaw. A red pencil is tucked behind his left ear. He wears a rumpled "
            "white lab coat, open over a mustard-yellow T-shirt with a dark brown round collar and a white "
            "outline print of a hand with two fingers raised. He wears no jewellery. The portrait ends at his "
            "chest."},
  {1082, 6, "A middle-aged man with fair skin, lined cheeks and neat, straight grey-brown hair cut in a mop "
            "falling to his brows and over his ears, with grey eyes. He wears a plain grey suit jacket with "
            "notched lapels, a white shirt with a pointed collar, and a tie striped diagonally in purple and dark "
            "grey. He wears no glasses or jewellery. The portrait ends at his chest."},
  {1083, 6, "Mr. Ono, a middle-aged man with tanned skin, brown eyes and a short goatee, who wears a dark iron "
            "samurai helmet: a domed bowl with a tall, curved gold crescent crest rising from the front, "
            "gold-tipped side flaps folded back by his temples, and a flared neck guard of riveted plates at the "
            "back. It is tied under his chin by a dark cord. Below it he wears a brown plaid suit jacket checked "
            "with darker lines, a white shirt with a pointed collar and a red tie. The portrait ends at his "
            "chest."},
  {1084, 6, "A man in his thirties with tanned skin, grey eyes and tousled, dark grey hair parted in the middle "
            "and falling over his ears, with light stubble along his jaw and chin. He wears a plain dark grey "
            "suit jacket with notched lapels, a white shirt with a pointed collar, and a red tie with a pattern "
            "of dark diamond checks. He wears no glasses or jewellery. The portrait ends at his chest."},
  {1085, 6, "Mr. Miyahara, a slight man with pale skin and a huge, round dark grey afro, grey eyes, small round "
            "copper wire-rimmed glasses and one large front tooth showing. He wears a mustard-yellow V-neck "
            "sweater over a green shirt with fine darker green pinstripes and a tall, upturned collar. He carries "
            "a large brown wooden set-square, a triangular ruler marked with measuring lines, held up over his "
            "shoulder. The portrait ends at his chest."},
  {1086, 6, "A police officer, a hard-faced man with tanned skin and short dark hair under a navy peaked police "
            "cap with a glossy black peak and band, a small silver button at the side, and a gold badge on the "
            "front: a star in a wreath of laurel leaves inside a dark circle. He wears a navy uniform jacket with "
            "silver shoulder buttons, a white shirt and a dark navy tie, with a gold badge on his chest and a "
            "grey radio microphone clipped at his left shoulder, its coiled cord hanging down. The portrait ends "
            "at his chest."},
  {1087, 6, "An old man with tanned, deeply wrinkled skin, grey eyes and long blue-grey hair combed back from a "
            "balding crown and tied in a bushy ponytail at the back of his head, with thick blue-grey eyebrows "
            "and a full, shaggy blue-grey beard and moustache. He wears a white high-collared jacket with a stand "
            "collar and round brass buttons, like a cook's coat. He wears no jewellery. The portrait ends at his "
            "upper chest."},
  {1181, 6, "A heavyset older man with fair skin, grey hair combed back from his forehead and over his ears, a "
            "round face with a double chin and a bushy grey moustache. He wears small round silver wire-rimmed "
            "glasses with red tips on the earpieces. He wears a dark grey suit jacket over a beige knitted V-neck "
            "sweater vest with ribbed edges, a white shirt with a pointed collar and a red tie. The portrait ends "
            "at his chest."},
  {1581, 6, "Ms. Toriumi, a young woman with fair skin and a chin-length, wavy light-brown bob with side-swept "
            "bangs, its ends flicking out. She has brown eyes, soft pink lipstick and a small pearl stud earring "
            "in her left ear. She wears a dusty rose suit jacket with notched lapels and a flap pocket, over a "
            "white blouse with a high stand collar and small round white buttons down the front. The portrait "
            "ends at her chest."},
  {1582, 6, "A woman with fair skin and brown hair swept to one side in a long fringe and pinned up into a bun at "
            "the back, brown eyes behind red oval-rectangular glasses, and pink lips. A necklace of pale pearls "
            "rests at the base of her throat. She wears a white jacket with a wide, open collar and notched "
            "lapels over a plain lilac top with a low, straight neckline. She wears no earrings. The portrait "
            "ends at her chest."},
  {1583, 6, "Mrs. Terauchi, a woman with fair skin and shoulder-length brown hair with side-swept bangs, flipped "
            "out at the ends and held back by a yellow hairband. She has brown eyes and pink lipstick. She wears "
            "an orange suit jacket with notched lapels and round buttons over a white top, with a pale yellow "
            "silk scarf tied loosely around her throat, its ends fluttering to one side. The portrait ends at her "
            "chest."},
  {1586, 6, "A woman with fair skin and long, wavy black hair swept back from her forehead and gathered loosely "
            "behind her, a few strands curling down by her face, with green eyes and peach lipstick. She wears "
            "thin, metal-framed rectangular glasses perched low on her nose. She wears a white long-sleeved top "
            "in a Chinese style, with a high mandarin collar and a diagonal front opening both piped in green. A "
            "thin gold ring necklace, a rigid hoop, sits loosely around her neck. The portrait ends at her chest."},
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
