// The dialogue portraits (included by portrait_text.cpp): every outfit of every character the game has a
// bust-up for (T_BU_<PC|SC|NC><char>_Pose<x>_C<outfit>, 156 outfits in 267 poses), each rendered from the game's own
// textures and looked at (tools/bustup/outfits.py, 2026-11-12). An outfit's text is the character's look in it; a
// pose's text is said after it. Character ids: 1 protagonist, 2 Yukari, 3 Junpei, 4 Akihiko, 5 Mitsuru, 6 Fuuka,
// 7 Aigis, 8 Ken, 9 Koromaru, 10 Shinjiro, 11 Metis, 12 Aigis (Episode Aigis); 1xx Social Links, 2xx story, 1xxx others.
const Outfit kOutfits[] = {
  {1, 0, "The protagonist, a slim, pale-skinned teenage boy with soft, dark slate-blue hair cut in a rounded "
         "bob, a stray tuft sticking up at the crown and a long fringe swooping across his right eye, so only "
         "his grey-blue left eye shows clearly. He wears a plain dark charcoal-grey top of smooth fabric with "
         "a small upright collar split into a narrow V at the throat, with no buttons, prints or trim. No "
         "jewellery or headphones, and the portrait ends at his upper chest. He gives off a quiet, aloof cool: "
         "hard to read, unbothered, a little mysterious behind that curtain of hair."},
  {2, 1, "Yukari, a slim teenage girl with short, chin-length light brown hair that flips outward at the ends, "
         "big brown eyes and small silver stud earrings. Her trademark pale grey choker with a heart-shaped "
         "buckle sits snug at her throat, over a crisp white short-sleeved button-up blouse with the collar "
         "open and a breast pocket: her school summer uniform. Fresh, neat and a little sporty, she has the "
         "bright, put-together look of the popular girl in class, with the heart choker adding a dash of cute "
         "attitude."},
  {2, 2, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
         "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. Around her neck is "
         "her pale grey leather choker with stitched edges and a heart-shaped buckle. Her school uniform is a "
         "soft pink ribbed cardigan, buttoned, over a white shirt whose wide, sharply pointed collar stands "
         "out over it, with a big bright red ribbon bow at the throat, its tails hanging down. All pink, red "
         "and polish, it's her signature look: pretty, spirited and very much the girl everyone notices."},
  {2, 5, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
         "silver stud earrings and her pale grey choker with a heart-shaped buckle. Her casual summer clothes "
         "are a light pink camisole with thin red-edged straps and a little cluster of red polka dots on the "
         "front, leaving her shoulders bare. It's a breezy, girlish off-duty look, easygoing and sunny, with "
         "the heart choker keeping her trademark touch."},
  {2, 6, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
         "silver stud earrings and her pale grey choker with a heart-shaped buckle. For winter she wears an "
         "off-the-shoulder pink ribbed knit top with a thick, fluffy white fur trim around the neckline, thin "
         "dark straps showing over her bare shoulders. Cosy yet stylish, it's a trendy, "
         "straight-out-of-a-fashion-magazine outfit, all soft pink and snowy fluff, that suits her fashionable "
         "streak perfectly."},
  {2, 51, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
          "silver stud earrings and her pale grey choker with a heart-shaped buckle. She wears her school "
          "uniform, a pink cardigan over a white shirt with a wide pointed collar and a big red bow, plus a red "
          "armband on her left arm printed in black with S.E.E.S. and Special Extracurricular Execution Squad. "
          "The armband turns her pretty schoolgirl look into something more official: the polished student with "
          "a secret night job."},
  {2, 52, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
          "silver stud earrings and her pale grey choker with a heart-shaped buckle. She's in combat gear: a "
          "dark grey and red zip-up jacket with a broad layered collar and a round black-and-white badge on the "
          "left sleeve, over a pink top, a white collared shirt and a large red bow. A quiver of black-shafted, "
          "white-fletched arrows rides over her right shoulder. She looks every inch the team's archer, sharp "
          "and ready, with her ribbon bow still tied just so."},
  {2, 102, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. She's in a "
           "swimsuit: a plain pale pink bikini top with thin teal halter straps tied behind her neck and meeting "
           "at a small knot at the front, her shoulders and arms bare. Pushed up on her head are chunky "
           "sunglasses with dark slate-grey rectangular frames, smoky brown lenses and small gold accents on the "
           "arms. No choker today. The shades perched in her hair give her a sunny, beach-day breeziness."},
  {2, 106, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes and small silver stud earrings. She's dressed as a maid: a "
           "black headband carrying a tall white frilled headdress of gathered lace, its scalloped edge punched "
           "with little eyelet holes, and a charcoal-black dress with puffed shoulders and a rounded white bib "
           "down the front, with a tall, sharply pointed white collar, fastened by four round black buttons. No "
           "choker. She's the picture of a prim, proper cafe maid, crisp and almost too tidy to be true."},
  {2, 154, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. She's wrapped in a "
           "dusty pink terry-cloth bath towel tucked straight across her chest, as at a hot spring, her "
           "shoulders and arms bare. No choker, and the portrait ends at the top of the towel. Without her usual "
           "accessories she looks plainer and more off-guard than usual, caught in a relaxed hot-spring moment."},
  {2, 155, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes and small silver stud earrings. She wears an inn's robes: a "
           "white cotton yukata crossed left over right, printed with a chain of grey outlined rings joined by "
           "small dark grey dots, with thin grey lines along the collar edges, and over it a loose, wide-sleeved "
           "slate-blue haori jacket of plain matte cloth, worn open. No choker. Bundled in the comfy inn robes "
           "she looks cosy and unwound, a cute, homey side of her."},
  {2, 156, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. Her summer festival "
           "yukata is a big pink and white checkerboard, crossed at the neck and sprinkled with small "
           "five-petalled flowers in yellow, pink and orange-red on slender green stems with long leaves; a "
           "glimpse of tan sash shows at her back near the bottom of the picture. No choker. Bright, bold and "
           "pretty, she looks ready for fireworks and festival stalls."},
  {2, 157, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes. She wears a "
           "formal New Year's kimono in deep rose pink printed with large white-outlined cherry blossoms, with a "
           "green-edged collar, a teal patterned sash at the bottom of the picture and a glimpse of a gold and "
           "dark zigzag obi bow behind her. Over her left ear sits a hair ornament of pink and white flowers "
           "with strands of dangling pink and silver petals. It's her most graceful, ladylike look, elegant and "
           "traditional, with the swaying hairpin adding a festive sparkle."},
  {2, 159, "Yukari, a slim, fair-skinned teenage girl with short, chin-length light brown hair, side-swept bangs "
           "and ends flicking outward, brown eyes, pink lips and small silver stud earrings. She wears her pale "
           "grey leather choker with stitched edges and a heart-shaped buckle, a coral-red long-sleeved "
           "crew-neck top, and over it a work apron striped vertically in pink and white, with dark red straps "
           "over her shoulders, white stitching along its top edge and straps, and a stitched front pocket. She "
           "looks cheerful and capable, the friendly part-timer behind a shop counter."},
  {2, 201, "Yukari, a slim teenage girl with short, chin-length light brown hair and brown eyes, wearing small "
           "silver stud earrings and her pale grey choker with a heart-shaped buckle. She wears her combat gear "
           "without the quiver: a dark grey and red zip-up jacket with a broad layered collar and a round "
           "black-and-white badge on the left sleeve, over a pink top, a white collared shirt and a large red "
           "bow. Battle-ready but unburdened, she looks brisk, capable and every bit the confident team member."},
  {3, 1, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
         "slate-blue baseball cap forward, a white knotwork emblem on its front. This is the summer school "
         "uniform: a white short-sleeved shirt hanging open, its wide collar flipped up, over a plain navy "
         "T-shirt. A silver chain necklace carries a small square tag set with red and green studs. Cap, "
         "popped collar and chain all say class clown with swagger: a goofy, easygoing joker trying hard to "
         "look cool."},
  {3, 2, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
         "slate-blue baseball cap with a white knotwork emblem. This is the winter school uniform: a "
         "charcoal-black blazer over a blue-grey collared shirt left unbuttoned at the neck, collar turned up, "
         "with a small green-and-yellow pin on the blazer's lapel. A silver chain necklace carries a small "
         "square tag set with red and green studs. He wears the uniform his own way, sloppy on purpose, with "
         "the cap and chain giving him that lovable slacker swagger."},
  {3, 5, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
         "slate-blue baseball cap with a white knotwork emblem. His casual summer clothes are a plain dark "
         "grey tank top that leaves his thin arms and shoulders bare, and a silver chain necklace with a small "
         "square tag set with red and green studs. It's a laid-back, hanging-out-at-the-arcade look, gangly "
         "and carefree, the guy who's always up for goofing around."},
  {3, 6, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
         "slate-blue baseball cap with a white knotwork emblem. For winter he wears a navy zip-up parka with a "
         "big shaggy brown fur collar, open over a plain white T-shirt, and his silver chain necklace with a "
         "small square tag set with red and green studs. That huge fur ruff swallows his skinny neck, giving "
         "him a bundled-up, streetwise look that's half cool and half comically cosy."},
  {3, 51, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
          "slate-blue baseball cap with a white knotwork emblem. He's in the winter school uniform, a "
          "charcoal-black blazer over a blue-grey shirt open at the neck with the collar up and a "
          "green-and-yellow lapel pin, with a red S.E.E.S. armband around his upper arm. A silver chain "
          "necklace carries a small square tag set with red and green studs. The armband lends his slouchy "
          "style a dash of official hero, which he clearly enjoys."},
  {3, 52, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
          "slate-blue baseball cap with a white knotwork emblem. Combat gear: no blazer, just a blue-grey "
          "collared shirt open at the neck, a red S.E.E.S. armband on his upper arm and a wide white leather "
          "strap with a steel buckle across his chest. Strapped to his back is a big two-handed sword, its long "
          "black-gripped hilt and steel cross-guard rising over his shoulder. A silver chain with a small "
          "studded square tag hangs at his throat. Part ball player, part wannabe knight, he looks gleefully "
          "ready to swing."},
  {3, 102, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, is "
           "bare-chested, as at the beach or in the water. He wears his slate-blue baseball cap backwards, its "
           "adjustable strap and metal buckle over his forehead, and his only other accessory is a silver chain "
           "necklace with a small square tag set with red and green studs. The backwards cap says it all: "
           "full-on beach-bum mode, goofy and carefree."},
  {3, 154, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, is "
           "bare-chested, as in a bath or at the beach. He wears his slate-blue baseball cap backwards, its "
           "adjustable strap and metal buckle over his forehead, and his only other accessory is a silver chain "
           "necklace with a small square tag set with red and green studs. Even at the bath he won't part with "
           "his cap, which is about as Junpei as it gets: relaxed, silly and totally himself."},
  {3, 155, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
           "slate-blue baseball cap with a white knotwork emblem. He's in an inn's robes: a white cotton yukata "
           "printed with a chain of grey rings, crossed loosely and low at the chest, under a slate-indigo haori "
           "jacket worn open. No necklace shows. Lounging in the robes with his cap still on, he looks loose, "
           "rumpled and comfortably at home, the guy most likely to start a late-night pillow fight."},
  {3, 202, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, wears his "
           "slate-blue baseball cap with a white knotwork emblem. No blazer, just a blue-grey collared shirt "
           "open at the neck with the collar up, a red S.E.E.S. armband on his upper arm and his silver chain "
           "necklace with a small square tag set with red and green studs. Shirt-sleeved and rumpled, he has the "
           "scrappy look of a guy ready for action, or at least one who wants you to think so."},
  {3, 901, "Junpei, a lanky young man with short dark hair, grey eyes and a scruffy thin goatee, in the summer "
           "school uniform: his baseball cap with its knotwork emblem, a white short-sleeved shirt hanging open "
           "over a navy T-shirt, and the silver chain with its studded square tag. This version is drawn in dim, "
           "cold bluish light, the cap and shirt deep in shadow, with dark rings under his eyes. His usual goofy "
           "energy looks drained and haunted here, worn down and sleepless."},
  {4, 1, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
         "sticking plaster slapped on the left side of his forehead. He wears the school summer uniform: a "
         "short-sleeved white shirt with a thin black ribbon tie, under a red knitted sweater vest with a "
         "small gold letter pin on the chest. Clean-cut and sharp-jawed, he has the cool, focused air of the "
         "star athlete, and that plaster is a boxer's badge of honour, hinting at a fighter who never stops "
         "training."},
  {4, 2, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
         "plaster on his forehead. He wears the school summer uniform: a white shirt with a thin black ribbon "
         "tie under a red knitted sweater vest with a small gold letter pin, and a tan leather holster strap "
         "buckled around his upper arm. The holster adds a hint of hidden danger to the clean-cut star "
         "athlete, a fighter who always seems ready for the next bout."},
  {4, 5, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
         "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
         "above his left eyebrow. His casual summer clothes are a deep red V-neck shirt in a soft knit with "
         "thin pale vertical pinstripes and a darker red band along the top of the shoulder, open at the "
         "throat. No jewellery. Simple and sporty, it's the no-fuss look of a guy who'd rather be out running "
         "than shopping, effortlessly sharp without trying."},
  {4, 6, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
         "plaster on his forehead. For winter he wears a light grey coat with tan-edged lapels over a dark "
         "charcoal shirt, and a long red ribbed scarf wrapped around his neck with the ends hanging down his "
         "chest. The bold red scarf against the pale coat and silver hair makes a striking, wintry picture, "
         "cool and a little dashing."},
  {4, 51, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
          "plaster on his forehead. He wears the school summer uniform, a white shirt with a thin black ribbon "
          "tie under a red knitted sweater vest with its small gold letter pin, with a tan holster strap on his "
          "upper arm and a red S.E.E.S. armband around the sleeve below it. Strapped and badged, he looks like "
          "the squad's seasoned senior, disciplined and itching for a fight."},
  {4, 52, "Akihiko, a lean, athletic young man with short, spiky silver-grey hair, grey eyes and a white "
          "plaster on his forehead. Over a white shirt, thin black ribbon tie and red knitted vest with its "
          "gold letter pin, he wears an open jacket with dark grey front panels, large round brass snaps and "
          "bright red sleeves, each sleeve marked with a round black-and-white quartered patch. Layered and "
          "bold, the battle jacket makes him look like a boxer stepping into the ring, all confidence and "
          "drive."},
  {4, 102, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He wears a plain white crew-neck T-shirt with a ribbed neckband, printed on "
           "the chest in grey with small lettering reading \"SCUBA DIVING\" above a large bold \"SOS\". No "
           "jewellery, and the portrait ends just below the lettering. It's a sporty, slightly dorky holiday tee "
           "that makes the usually intense boxer look refreshingly casual."},
  {4, 154, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He is shirtless, as at a bath or hot spring, with nothing else worn and no "
           "jewellery or towel showing, and the portrait ends at his upper chest. Even here the forehead plaster "
           "stays put, a reminder that he's a boxer through and through."},
  {4, 155, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He wears an inn's yukata: a white cotton robe crossed loosely at the neck, "
           "printed with a chain of grey outlined rings joined by small dark grey dots, with thin grey lines "
           "along the collar, and over it a loose, wide-sleeved slate-blue haori jacket worn open. In the "
           "relaxed robes, plaster still on his brow, he looks unusually at ease, a tough guy taking a rare "
           "night off."},
  {4, 159, "Akihiko, a lean, athletic, fair-skinned young man with short, spiky silver-grey hair swept up and "
           "back in ragged tufts, grey eyes, a strong jaw and a white rectangular plaster stuck at an angle "
           "above his left eyebrow. He wears a white collarless shirt with a narrow band neck and a short open "
           "slit at the throat, marked with faint vertical pinstripes, and over it a dark red bib apron with "
           "straps over his shoulders, its edges and front pocket outlined in dashed stitching. It's a neat, "
           "workmanlike look: the earnest, no-nonsense type who'd treat even a part-time job as serious "
           "training."},
  {5, 1, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair tumbling past her "
         "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
         "eyes and soft coral-pink lipstick. She carries herself like the heiress she is, cool, composed and a "
         "touch regal. In the school uniform: a white blouse with a high, close collar, full puffed sleeves "
         "and fine vertical pleats, and a large red ribbon bow at the throat, its wide tails spilling down her "
         "chest like a flourish. No jewellery."},
  {5, 2, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair tumbling past her "
         "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
         "eyes and soft coral-pink lipstick. Even in school clothes she looks like she runs the place, which, "
         "as student council president, she more or less does. A white blouse with a high, close collar, full "
         "puffed sleeves and fine vertical pleats, and a big red ribbon bow at the throat with wide tails down "
         "her chest. No jewellery."},
  {5, 5, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair tumbling past her "
         "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
         "eyes and soft coral-pink lipstick. Her summer look is pure understated elegance: a sleeveless white "
         "turtleneck in a fine vertical rib knit, its tall neck folded over, cut in deep at the armholes so "
         "her shoulders and arms are bare. No print, no trim, no jewellery; the red hair does all the talking."},
  {5, 6, "Mitsuru, a tall, fair-skinned young woman with long, wavy crimson hair falling past her shoulders, a "
         "long fringe sweeping over her left eye, red eyes and coral-pink lipstick. Her winter look is sleek "
         "and commanding, a little biker-chic aristocrat: a black leather double-breasted coat with wide "
         "lapels and silver snap studs, over a cream shirt whose tall collar stands up crisply around her "
         "throat. Cool, expensive and not to be trifled with."},
  {5, 51, "Mitsuru, a tall, fair-skinned young woman with long, wavy crimson hair falling past her shoulders, a "
          "long fringe sweeping over her left eye, red eyes and coral-pink lipstick. She wears the school "
          "uniform, a white high-collared blouse with puffed sleeves and a large red bow at the throat, with a "
          "red lettered armband around her upper arm, the badge of office that says she is in charge here. "
          "Crisp, authoritative and every inch the student council president."},
  {5, 52, "Mitsuru, a tall, fair-skinned young woman with long, wavy crimson hair falling past her shoulders, a "
          "long fringe sweeping over her left eye, red eyes and coral-pink lipstick. Dressed for the Dark Hour: "
          "over her white puffed-sleeve blouse and large red bow she wears a black high-collared vest with a "
          "strap across the shoulder fastened by a brass snap, like combat gear. The frilly bow against the "
          "hard black vest gives her the air of a noblewoman ready to lead a charge."},
  {5, 102, "Mitsuru, a tall, poised, fair-skinned young woman with long, wavy crimson hair tumbling past her "
           "shoulders in heavy waves, one curl flicking outward, a long fringe sweeping over her left eye, red "
           "eyes and soft coral-pink lipstick. She wears a swimsuit: a plain white bikini top with thin white "
           "straps and a small knot at the centre front. A large bright red hibiscus with a long yellow-tipped "
           "stamen is pinned at her left strap, a splash of tropical colour on an otherwise cool, classy look. "
           "No jewellery."},
  {5, 154, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair gathered into a high ponytail that spills in big loose curls, her long fringe still "
           "over her left eye and loose strands framing her cheeks. At the hot spring she is wrapped in a dusty "
           "pink terry-cloth bath towel tucked straight across her chest; her shoulders and arms are bare. The "
           "portrait ends at the top of the towel. Hair up, she looks unexpectedly relaxed."},
  {5, 155, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair gathered into a high ponytail that spills in big loose curls, her long fringe still "
           "over her left eye. In an inn's yukata, a white cotton robe crossed at the neck and printed with a "
           "chain of grey outlined rings joined by small dark grey dots, thin grey lines along the collar, under "
           "a loose, wide-sleeved slate-blue haori worn open. Off duty, yet somehow still dignified."},
  {5, 156, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair gathered into a high ponytail that spills in big loose curls, her long fringe still "
           "over her left eye. Her festival yukata is white cotton with thin paired pink stripes, printed with "
           "sprays of slender green leaves and hanging clusters of purple and pink bell flowers with yellow "
           "centres, pink petals drifting between them. A green sash shows at her back. Graceful and "
           "summery-refined."},
  {5, 157, "Mitsuru, a tall, fair-skinned young woman with red eyes and coral-pink lipstick, her crimson hair "
           "pinned up and crowned with a large white silk flower, red camellias, looped gold cords and a hanging "
           "gold tassel. Her New Year kimono is dark green silk with large red camellia blossoms and scattered "
           "orange dots, red and gold patterned collars and a gold-patterned sash. Lavish, traditional and "
           "utterly regal, she looks like old money at its most festive."},
  {5, 159, "Mitsuru, a tall, poised, fair-skinned young woman with red eyes and soft coral-pink lipstick, her "
           "crimson hair gathered into a high ponytail that spills in big loose curls, her long fringe still "
           "over her left eye. She wears a long-sleeved white turtleneck sweater in a fine vertical rib knit, "
           "and over it a dark brown bib apron on olive-khaki straps with dashed stitching. The heiress in an "
           "apron, ready to cook, is a charmingly unlikely sight."},
  {6, 1, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
         "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. She has a soft, "
         "gentle, bookish air, the shy girl you would find quietly tinkering in the corner. In her school "
         "summer uniform: a white long-sleeved blouse with a wide, round neckline edged in scalloped lace and "
         "tied with a thin white ribbon bow, over a fitted bright teal turtleneck. No jewellery."},
  {6, 2, "Fuuka, a slight, fair-skinned girl with a short teal bob, choppy straight-cut bangs and large grey "
         "eyes, sweet and a little timid-looking. In her school winter uniform: a dark charcoal blazer with "
         "silver piping, a strap with two round silver buttons across the chest and a red school crest, over a "
         "high teal turtleneck with a faint lace pattern. A thin white ribbon is tied at the back of her neck, "
         "its ends trailing over one shoulder, a delicate touch that suits her."},
  {6, 5, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
         "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. Her summer "
         "outfit is soft and sweet: a sleeveless mint-teal dress with wide ruffled shoulder straps and a "
         "square neckline edged with a ruffle and outlined by a thin gold line, leaving her arms and "
         "collarbones bare. Light, frilly and gentle, like a breezy afternoon. No jewellery."},
  {6, 6, "Fuuka, a slight girl with a short teal bob, choppy straight-cut bangs and large grey eyes, bundled "
         "up cosy for winter. A soft green poncho with white embroidered flower motifs has a wide cream collar "
         "trimmed with fur, tied at the front with a white ribbon ending in fluffy pompoms, over a "
         "charcoal-grey ribbed turtleneck sweater. The whole look is snug and storybook-sweet, a little like a "
         "girl out of a winter fairy tale."},
  {6, 52, "Fuuka, a slight girl with a short teal bob, choppy straight-cut bangs and large grey eyes, kitted "
          "out as the team's navigator for the Dark Hour. A heavy charcoal greatcoat with a tall stand-up "
          "collar closed by a buttoned tab, brass cross-marked buttons, gold cords at one shoulder and a short "
          "shoulder cape sits over a teal top. A silver headset with chunky ear cups and a thin microphone arm "
          "rides over her hair, and she wears dark fingerless gloves. Small girl, serious gear."},
  {6, 102, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
           "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. She wears a "
           "swimsuit: a plain mint-teal top cut straight across, with no print or trim, on thin white straps "
           "that tie in a halter bow behind her neck. Her shoulders and arms are bare. Simple and modest, very "
           "much her quiet style. No jewellery."},
  {6, 154, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
           "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. At the hot "
           "spring she is wrapped in a dusty pink terry-cloth bath towel tucked straight across her chest; her "
           "shoulders and arms are bare. The portrait ends at the top of the towel. With her neat little bob she "
           "keeps her usual soft, quiet air."},
  {6, 155, "Fuuka, a slight, fair-skinned teenage girl with a short teal bob, its choppy, straight-cut bangs "
           "high on her forehead and its jagged ends curling in at her jaw, and large grey eyes. In an inn's "
           "robes: a white cotton yukata crossed at the neck, printed with a chain of grey outlined rings joined "
           "by small dark grey dots, thin grey lines along the collar, under a loose, wide-sleeved slate-blue "
           "haori of plain matte cloth, worn open. Snug and sleepy-soft, ready for tea."},
  {6, 156, "Fuuka, a slight, pale-skinned teenage girl with a short teal bob in feathered layers to her jaw, "
           "straight-cut bangs and large grey eyes, wearing no jewellery. Her festival yukata is mid-blue "
           "cotton, crossed left over right at a V neckline with a sliver of white underlayer, printed with big "
           "sunflowers, golden petals round brown seed-dotted centres, over faint pale green rings. An orange "
           "obi bow peeks out at the back; the portrait ends at her upper chest. Cheerful and sunny on her."},
  {6, 157, "Fuuka, a slight girl with a short teal bob, choppy straight-cut bangs and large grey eyes, dressed "
           "up prettily for New Year. Her kimono is bright teal with white, yellow and red chrysanthemums, its "
           "collar layered in cream, coral and olive, with a pink obi in a red diamond pattern and a yellow bow "
           "at the back. Above one ear sits an ornament of an orange and a yellow fabric flower with green cords "
           "ending in small silver beads. Bright, festive and sweetly demure."},
  {6, 901, "Fuuka, shown only as a flat dark grey silhouette, her features shaded over by a darker band so only "
           "faint outlines of eyes and mouth remain, a mysterious, unknown figure. The outline shows her short, "
           "feathered bob, the tall stand-up neck of her school winter uniform closed at the throat, the "
           "shoulders of the blazer with a faint button or two, and a thin ribbon tied at the back of her neck, "
           "its loops and ends trailing behind one shoulder. No colour or pattern can be made out."},
  {7, 2, "Aigis, a girl-shaped android with short pale blonde hair and bright, clear blue eyes. A gold "
         "headband cut with triangular openings crosses her hair, white mechanical ear units with red "
         "three-lobed centres replace her ears, and her white neck shows seam lines. She has a serene, "
         "doll-like stillness, earnest and a little uncanny. Here she wears the school winter uniform: a black "
         "blazer with silver piping, a large ruffled red ribbon bow at the throat and a round red-and-black "
         "Gekkoukan badge with a quartered emblem."},
  {7, 6, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
         "triangular openings and white mechanical ear units with red centres; seams line her white neck. Her "
         "winter clothes are prim and old-fashioned, almost like a porcelain doll dressed for tea: an open "
         "teal-blue jacket over a white blouse with a ruffled high collar, a loose white cravat bow and a "
         "frilled front. Proper, pretty and very earnest."},
  {7, 52, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
          "triangular openings and white mechanical ear units with red centres. This is her battle form, and "
          "she looks every bit the weapon: a smooth white body with gold, openwork shoulder joints and dark "
          "round hinge caps, softened by a large ruffled red ribbon bow at her throat, and a red armband on her "
          "upper arm with a black-and-white emblem and the letters S.E.E.S. Sweet and steely at once."},
  {7, 155, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres; the seams of her white neck "
           "show. She wears an inn's robes: a white cotton yukata printed with a chain of grey linked rings, "
           "under a loose, wide-sleeved slate-blue haori jacket worn open. The cosy robe on a machine girl makes "
           "her look endearingly earnest, as if she studied how relaxing works."},
  {7, 156, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres; seams line her white neck. Her "
           "summer festival yukata is deep blue, printed with large white blossoms with orange centres on "
           "slender stems with leaves and buds, and a red obi bow shows behind her back. Fresh and pretty, with "
           "that calm, doll-like poise that is all her own."},
  {7, 157, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. Her New Year kimono is pale "
           "pink with large cherry blossoms, its collar edged in red and light blue over a gold patterned "
           "underlayer, with a green obi and a gold patterned bow at the back. A cluster of pink cherry blossoms "
           "with dangling gold strips is pinned beside her ear unit. Soft, springlike and delicately festive."},
  {7, 201, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. Her bare battle body shows: "
           "smooth white plating with seam lines along the neck and chest, and gold, openwork shoulder joints "
           "with dark round hinge caps. No bow and no armband, nothing to soften the machinery; she looks "
           "stripped down, clinical and very much a built thing."},
  {7, 202, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. Her battle body: smooth white "
           "plating with gold, openwork shoulder joints and dark round hinge caps, and a large ruffled red "
           "ribbon bow at her throat. Unlike her usual battle look there is no S.E.E.S. armband, so she reads as "
           "pure machine with a single girlish flourish."},
  {7, 203, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres, in her battle body after heavy "
           "damage. Her white plating, gold shoulder joints, face and hair are smeared with soot and scratches, "
           "and the white trim under her red ribbon bow is torn and ragged. Battered and worn down, she looks "
           "like she has fought to the very end."},
  {7, 204, "Aigis, a girl-shaped android with short pale blonde hair, bright blue eyes, a gold headband cut with "
           "triangular openings and white mechanical ear units with red centres. She wears a loose pale blue top "
           "like a nightdress, with a white ruffled round collar and a buttoned white placket edged with ruffles "
           "down the front; the seams of her white neck show above it. Soft and homely, it makes her look "
           "gentle, almost fragile."},
  {8, 1, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its bangs swept to "
         "one side and its ends flicking outward over his ears, and large brown eyes. He has the earnest, "
         "trying-hard look of a kid who wants very much to be taken seriously. A sleeveless orange hooded vest "
         "hangs unzipped, silver zip edge showing and hood bunched at his neck, over a white collared shirt "
         "with small buttons and a thin black string tie in a loose bow, its long ends down his chest. No "
         "jewellery."},
  {8, 2, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
         "outward over his ears, and large brown eyes. His school winter uniform: an open dark grey zip jacket "
         "with a silver zip, a diagonal zip pocket, a small brass snap and a round badge, a red ring lettered "
         "in gold Gekkoukan Private Elementary School round a black-and-white quartered emblem. Under it an "
         "orange hoodie, hood spread at his neck, and a white collared shirt with a thin black string tie. "
         "Neat and serious."},
  {8, 5, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
         "outward over his ears, and large brown eyes. In casual clothes he looks every bit the sporty kid: a "
         "zip-up track jacket with a silver zip, its lower front and sleeve panels burnt orange and its "
         "shoulder yoke, upper sleeves and zip strips mustard yellow, every seam edged with white piping, over "
         "a dark grey crew-neck T-shirt. No jewellery or accessories."},
  {8, 6, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
         "outward over his ears, and large brown eyes. He is bundled up adorably for winter in a thick, chunky "
         "cream knit sweater whose tall, loose rolled turtleneck swallows him up to the chin, its front worked "
         "in vertical ribs and cables. The shoulders are tan and the sleeves a deeper caramel brown, sewn on "
         "in raglan panels. Warm, cosy and very much a little kid. No accessories."},
  {8, 52, "Ken, a small, fair-skinned boy with a soft, tousled light-brown bob and large brown eyes, suited up "
          "and determined for battle. He wears his school outfit, a dark grey zip jacket over an orange hoodie "
          "and a white collared shirt with a thin black string tie, with a black strap over his shoulder and a "
          "big red armband around his upper arm bearing a black-and-white emblem. A kid trying hard to look "
          "like a soldier, and nearly pulling it off."},
  {8, 159, "Ken, a small, fair-skinned boy of about ten with a soft, tousled light-brown bob, its ends flicking "
           "outward over his ears, and large brown eyes. He wears a plain lemon-yellow T-shirt with a dark grey "
           "ribbed crew neck and short sleeves, under a rust-brown apron on narrow shoulder straps, its edges "
           "and straps lined with dashed white stitching and a stitched pocket across the bib, ready to cook. A "
           "diligent little helper in the kitchen. The portrait ends at his upper chest."},
  {9, 6, "Koromaru, a white Shiba Inu with striking red eyes, upright pointed ears with grey insides, two grey "
         "brow spots, a black nose, a white face and chest, and soft grey shading over his neck, back and "
         "thick curled tail. He's every inch the loyal good boy dressed for duty: a sleeveless cream-beige "
         "canvas vest with dashed stitching, and a thick grey metal collar with a round housing capped by a "
         "glowing blue glass dome, behind it a little white angel wing on a white disc held by a slotted pale "
         "green screw cap."},
  {9, 52, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears, two grey brow spots and a thick "
          "curled tail, kitted out like a tiny canine commando. Over the tan vest and grey mechanical collar "
          "with its glowing blue dome and small white wing he wears a black harness with brass-buckled straps, "
          "a round brass tag dangling at his chest, a white canister with red stripes strapped to his back and "
          "a red armband on his foreleg. Pure earnest, fluffy bravery."},
  {9, 201, "Koromaru, a white Shiba Inu with red eyes, upright pointed ears with grey insides, two grey brow "
           "spots, a black nose and a white face. Grey shading runs over his neck, shoulders and back, his chest "
           "is a fluffy paler white ruff, and his thick tail curls up behind him. He's just a dog here, "
           "wonderfully plain and fluffy: none of his tan vest or metal collar with the blue dome and white "
           "wing, only that thick, huggable fur."},
  {10, 6, "Shinjiro, a tall young man with shoulder-length, messy brown hair falling over grey eyes, and a long "
          "jaw. He has a rough, brooding, keep-your-distance air, all street-tough loner. A dark grey knit "
          "beanie with a small metal plate on its ribbed brim is pulled low, and a long maroon-red "
          "double-breasted pea coat is buttoned up to wide dark grey lapels and a turned-up collar studded with "
          "small silver rivets, like armour against the world."},
  {10, 52, "Shinjiro, a tall young man with shoulder-length, messy brown hair falling over grey eyes, and a long "
           "jaw. In his battle look the gruff loner edge is sharper: the dark grey knit beanie with its small "
           "metal plate stays pulled low, and the long maroon-red coat with wide, rivet-studded dark grey lapels "
           "hangs open over a black high-necked ribbed sweater, a metal belt buckle glinting at the bottom of "
           "the picture."},
  {10, 159, "Shinjiro, a tall young man with fair skin, a long jaw and grey eyes, his shoulder-length brown hair "
            "falling messily over his face and out behind his neck. He still looks like the toughest guy in the "
            "room, which makes the outfit a delight: his dark grey knit beanie pulled low, its ribbed brim "
            "fronted by a riveted metal plate engraved with a slanted line, a plain charcoal mock-neck top, and "
            "over it a tan canvas cooking apron on narrow straps, with dashed stitching and a bib pocket."},
  {11, 52, "Metis, a slender android girl with a short, sleek blue-black bob and red eyes, cool and quietly "
           "intense. On her head sits a large gold-framed crown of red butterfly-wing panels with round red ear "
           "discs pierced by gold spikes; in some looks it folds down into a gold-and-red butterfly visor that "
           "hides her face entirely. Her body is a seamless dark grey suit with white jointed shoulder parts, "
           "and a red armband with a black-and-white emblem and lettering is fixed to her upper arm."},
  {11, 201, "Metis, a slender android girl with a short, sleek blue-black bob and red eyes, cool and quietly "
            "intense. On her head sits a large gold-framed crown of red butterfly-wing panels with round red ear "
            "discs pierced by gold spikes; in some looks it folds down into a gold-and-red butterfly visor that "
            "hides her face entirely. Her body is a seamless dark grey suit with white jointed shoulder parts, "
            "sleek and unadorned; unlike her battle look she wears no armband."},
  {12, 2, "Aigis, a girl-shaped android with fair, smooth skin, short blond hair in a choppy bob with long "
          "side-swept bangs, and bright blue eyes, a doll-like stillness about her. A gold headband of open "
          "triangular lattice sits on her head, round white mechanical ear units with red three-lobed centres "
          "cover her ears, and her neck is white and jointed. She wears the Gekkoukan winter uniform: a black "
          "blazer with grey piping, a white collar, a large red ruffled bow, a silver zip pull and the round "
          "red-ringed Gekkoukan Private High School badge."},
  {12, 6, "Aigis, a girl-shaped android with fair, smooth skin, short blond hair in a choppy bob with long "
          "side-swept bangs, and bright blue eyes. A gold lattice headband, round white mechanical ear units "
          "with red three-lobed centres and a white jointed neck give her away. Her casual look is charmingly "
          "old-fashioned, like a little aristocrat: a white blouse with a high frilled collar and pleated "
          "front, a large flowing white cravat tied in a bow with long tails, and an open teal jacket with wide "
          "lapels."},
  {12, 52, "Aigis, a girl-shaped android with short blond hair in a choppy bob, bright blue eyes, a gold lattice "
           "headband, white mechanical ear units with red centres and a white jointed neck. Her robot body "
           "shows: a smooth white chest with a large red ruffled bow at the throat, bulky gold mechanical "
           "shoulder joints with black ribbed cylinders and open lattice plates, and a red armband with a "
           "black-and-white emblem and lettering on her upper arm. Every inch the earnest guardian machine."},
  {12, 53, "Aigis, a girl-shaped android with fair, smooth skin, short blond hair in a choppy bob with long "
           "side-swept bangs, and bright blue eyes. A gold lattice headband, round white mechanical ear units "
           "with red three-lobed centres and a white jointed neck frame her face. Her robot body is plainly on "
           "show, part schoolgirl, part war machine: a smooth white chest with a large red ruffled bow at the "
           "throat, and bulky gold shoulder joints with black ribbed cylinders, round black caps and open "
           "lattice plates. No armband."},
  {12, 54, "Aigis, a girl-shaped android with short blond hair, bright blue eyes, white mechanical ear units "
           "with red centres and a white jointed neck, dressed for a siege. A riveted steel visor band edged in "
           "gold crosses her forehead over the lattice headband, a steel gorget hangs on chains over her red "
           "bow, a dark harness bristles with rows of brass bullet cartridges, and ribbed gold armour caps her "
           "shoulders, with a red armband bearing a black-and-white emblem. A knight and a gunship in one."},
  {12, 201, "Aigis, a girl-shaped android with short blond hair, bright blue eyes, white mechanical ear units "
            "with red centres and a white jointed neck, dressed for a siege. A riveted steel visor band edged in "
            "gold crosses her forehead over the lattice headband, a steel gorget hangs on chains over her red "
            "bow, a dark harness bristles with rows of brass bullet cartridges, and ribbed gold armour with "
            "lattice plates caps both shoulders, with no armband. A knight and a gunship in one."},
  {101, 1, "Kenji, a slim teenage boy with fair skin, short, messy light-brown hair falling in uneven bangs over "
           "his forehead and ears, and brown eyes. He has an easygoing, slightly scruffy class-clown energy, the "
           "guy always chasing his next big crush. He wears the school summer uniform: a short-sleeved white "
           "shirt with a pointed collar, small white buttons and a breast pocket, with a thin black ribbon tie "
           "knotted loosely, its long ends hanging crookedly. No jewellery or accessories."},
  {101, 2, "Kenji, a slim teenage boy with fair skin, short, messy light-brown hair falling in uneven bangs, and "
           "brown eyes, with a casual, slightly scruffy class-clown air. He wears the school winter uniform a "
           "little carelessly: a black blazer with notched lapels and a silver strip running diagonally across "
           "the chest, a white collared shirt with a thin black ribbon tie hanging loose, a small half-green, "
           "half-yellow pin on the lapel, and the round red-ringed Gekkoukan Private High School badge on the "
           "chest."},
  {101, 6, "Kenji, a slim teenage boy with fair skin, short, messy light-brown hair falling in uneven bangs, and "
           "brown eyes. His casual clothes shout cheerful and loud, just like him: an open bright mustard-yellow "
           "zip jacket with a silver zip, a high stand-up collar lined in white and a row of small snap buttons, "
           "over a dark grey V-neck T-shirt layered on a white crew-neck one whose collar shows at his throat. "
           "No jewellery or accessories."},
  {101, 158, "Kenji, a slim teenage boy with fair skin, short light-brown hair poking up over a cap band and "
             "falling over his ears, and brown eyes. He's comically decked out in a fast-food work uniform, every "
             "inch the part-timer: an orange visor with a long pale yellow brim and a big looping cursive W logo "
             "on its front, and an orange short-sleeved shirt with a pale yellow pointed collar, button placket, "
             "pocket strip and sleeve bands."},
  {102, 1, "Hidetoshi, a lean teenage boy with fair skin, dark grey hair slicked back from a high forehead, a "
           "few strands curling forward and the ends flicking out at his nape, and narrow grey eyes. He has the "
           "stiff, self-important look of a rule-enforcer who takes himself very seriously. He wears the school "
           "summer uniform: a short-sleeved white shirt with a pointed collar and breast pocket, a thin black "
           "ribbon tie, and a mustard-yellow armband with large black kanji on his upper arm."},
  {102, 2, "Hidetoshi, a lean teenage boy with fair skin, dark grey hair slicked back from a high forehead with "
           "a few strands curling forward, and narrow grey eyes. Buttoned-up and officious, he looks every bit "
           "the disciplinary committee man. He wears the school winter uniform: a black blazer with a silver "
           "strip down its front, a small green-and-yellow lapel pin and the red school crest, over a white "
           "shirt and thin black ribbon tie, with a mustard-yellow armband with black kanji over the sleeve."},
  {103, 6, "Bunkichi, an elderly man with wild, thinning white hair sticking out in tufts, bushy white eyebrows, "
           "a white moustache, and black rectangular glasses slipping down his nose, with a dark spot on his "
           "bald forehead. He's a cosy, kindly grandpa through and through. He wears a cream knitted sweater "
           "with a dark grey ribbed turtleneck collar and red and blue zigzags across the chest and arms, and "
           "holds a dark wooden tobacco pipe."},
  {104, 6, "Mitsuko, a small, very old woman with pale skin, a heavily wrinkled round face with sagging cheeks "
           "and pouched eyes, and thinning grey hair parted in the middle and cut to chin length. She's a sweet, "
           "soft little granny who looks like she'd press snacks on you. She wears a lilac blouse with a round "
           "collar and small buttons, printed all over with grey-blue leafy sprigs and tiny cream flowers, under "
           "a dark olive-green work apron whose broad straps fasten with round brass buttons. No jewellery."},
  {105, 101, "Kazushi, an athletic teenage boy with lightly tanned skin, short, spiky dark grey hair standing up "
             "on top and cut close at the sides, and dark eyes. He radiates sporty, cocky jock energy, a guy who "
             "lives on the track. He wears a grey track jacket zipped right up to a tall stand-up collar with a "
             "silver zip pull at his throat, twin white stripes running down either side of the zip and over his "
             "shoulders, and red panels low on each side of the chest. No jewellery or accessories."},
  {105, 151, "Kazushi, an athletic teenage boy with lightly tanned skin, short, spiky dark grey hair standing up "
             "on top and cut close at the sides, and dark eyes. Kitted out for practice, he's all sporty, "
             "competitive energy. He wears a dark grey sleeveless basketball-style tank top with a deep scooped "
             "neck, its neckline and armholes edged in red trim and a black-and-red patch at one shoulder, leaving "
             "his shoulders and arms bare. No jewellery or accessories."},
  {106, 1, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
           "ponytail sticking up at the back of her head, a slim pink hairpin above her left temple, and brown "
           "eyes. Sun-kissed and boyish, she has a bright, sporty, big-sister energy. She wears the school "
           "summer uniform: a short-sleeved white blouse with a stand-up collar, small white buttons and a "
           "breast pocket, and a large red ribbon bow with ruffled tails at the collar. No earrings or other "
           "jewellery."},
  {106, 2, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
           "ponytail sticking up at the back, a slim pink hairpin above her left temple, and brown eyes, sporty "
           "and bright. She wears the school winter uniform neatly: a black jacket with grey piping, a front "
           "flap fastened by silver buttons and a small silver zip pull, over a white collared blouse with a "
           "large red ribbon bow, and the round red-ringed Gekkoukan Private High School badge on the chest."},
  {106, 6, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
           "ponytail sticking up at the back, a slim pink hairpin above her left temple, and brown eyes. Her "
           "off-duty look is easy, sporty and tomboyish: an open black zip-up hoodie with a silver zip, its hood "
           "lined in white and folded around her neck and a white stripe down each sleeve, over a coral-red top "
           "with a wide scooped neckline. No jewellery."},
  {106, 101, "Yuko, a teenage girl with tanned skin, short dark blue-black hair swept over to one side, a small "
             "ponytail sticking up at the back, a slim pink hairpin above her left temple, and brown eyes. Dressed "
             "as the athletics team's go-getter manager, she looks ready to run laps herself: a grey track jacket "
             "zipped up to a tall stand-up collar with a silver zip pull at her chin, twin white stripes down "
             "either side of the zip and over her shoulders, and red panels low on each side of the chest. No "
             "jewellery."},
  {107, 6, "Maya, an online game avatar drawn as a translucent green hologram made of flat, low-polygon shapes, "
           "crossed by horizontal scan lines and glowing pink and cyan at its edges. It's gloriously "
           "retro-digital, like a character beamed out of an old PC game. The figure has a short, flicked-out "
           "bob of hair, one visible eye, and a bulky high-collared coat with stripes and diamond shapes across "
           "it."},
  {108, 1, "Keisuke, a slim teenage boy with fair skin and thick, soft, wavy light-brown hair that falls in "
           "loose curls over his forehead, ears and the back of his neck, and grey eyes behind thin black "
           "rectangular glasses. He looks gentle and bookish, the quiet, kind type you'd trust with a secret. He "
           "wears the school summer uniform: a crisp white collared shirt with small white buttons and a breast "
           "pocket, and a thin dark green-black ribbon tied loosely at the collar. The portrait ends at "
           "mid-chest."},
  {108, 2, "Keisuke, a slim teenage boy with fair skin and thick, soft, wavy light-brown hair falling in loose "
           "curls, and grey eyes behind thin black rectangular glasses, gentle and bookish. He wears the school "
           "winter uniform tidily: a charcoal-black blazer with notched lapels and thin silver-white piping, a "
           "small green-and-cream bar pin on the lapel, and the school crest, a red ring lettered Gekkoukan "
           "Private High School around a black-and-white quartered circle, over a white shirt and a thin dark "
           "green-black ribbon tie."},
  {108, 6, "Keisuke, a slim teenage boy with fair skin and thick, soft, wavy light-brown hair falling in loose "
           "curls over his forehead, ears and neck, and grey eyes behind thin black rectangular glasses. Off "
           "duty he's pure gentle-nerd comfort: a collared button-up shirt in a bold plaid of red, sky blue, "
           "white and grey checks with small grey buttons, open at the neck over a dark green T-shirt whose "
           "round collar just shows. The portrait ends at mid-chest."},
  {109, 1, "Chihiro, a slender teenage girl with fair skin and very long, straight brown hair parted in the "
           "middle and swept back off a high forehead, falling down her back with long strands framing her face. "
           "Brown eyes behind black rectangular glasses and soft pink lips give her a shy, studious, terribly "
           "proper air, the timid treasurer of the student council. She wears the school summer uniform: a white "
           "collared blouse with small white buttons and a large, full red bow spreading over her chest."},
  {109, 2, "Chihiro, a slender teenage girl with fair skin and very long, straight brown hair parted in the "
           "middle and swept back off a high forehead, long strands framing her face. Brown eyes behind black "
           "rectangular glasses and soft pink lips give her a shy, prim, studious air. She wears the school "
           "winter uniform very properly: a charcoal-black jacket with thin silver-white piping and the red "
           "school crest just visible, over a white collared blouse with white cuffs and a large, full red bow "
           "at the collar."},
  {109, 6, "Chihiro, a slender teenage girl with fair skin and very long, straight brown hair parted in the "
           "middle and swept back off a high forehead, long strands framing her face, with brown eyes behind "
           "black rectangular glasses and soft pink lips. Out of uniform she's sweet and softly old-fashioned, "
           "dressed like a shy librarian: a warm tan-orange cardigan with a frilled ruffle down its button "
           "front, over a cream top with a high, close collar whose upper edge is cut in a zigzag."},
  {110, 6, "Maiko, a little girl of about eight with fair skin, a round face and big brown eyes, as cute as a "
           "button. Her brown hair is pulled smoothly back into a braided bun wrapped with a ruffled pink "
           "scrunchie. She wears an open, slightly oversized denim-blue shirt with a pointed collar, silver snap "
           "buttons and rolled sleeves over a plain pink T-shirt, and on her back a glossy red leather randoseru "
           "school backpack with red straps, silver fittings and a white cord dangling from a clip."},
  {111, 6, "Pharos, a small, slight, pale boy who looks about ten, with messy, tousled dark blue-grey hair cut "
           "short with ragged bangs, and large, bright cyan-blue eyes, a small dark mole under his left eye. "
           "There's something quietly eerie and otherworldly about him, like a sweet child who isn't quite "
           "there. He wears a long-sleeved shirt in wide horizontal stripes of dark grey and off-white, almost "
           "like old-fashioned pyjamas, with a round neckline and a short placket of three small grey buttons."},
  {112, 1, "Bebe, a slim teenage boy with light skin, a neat, rounded blond bowl cut with thick straight bangs "
           "down to his eyebrows, and pale turquoise-blue eyes. He's a sunny, starry-eyed exchange student "
           "utterly in love with all things Japanese, and it shows: he carries an open folding fan of black "
           "paper on black ribs, painted with two gold butterflies above sweeping gold arcs and scattered gold "
           "dots. He wears the school summer uniform: a short-sleeved white shirt with a breast pocket and a "
           "thin black ribbon tie."},
  {112, 2, "Bebe, a slim teenage boy with light skin, a neat, rounded blond bowl cut with thick straight bangs, "
           "and pale turquoise-blue eyes, a cheerful exchange student wild about Japanese culture. He carries an "
           "open black folding fan painted with two gold butterflies above sweeping gold arcs and gold dots. He "
           "wears the school winter uniform: a charcoal-black blazer with silver-white piping, a green-and-cream "
           "lapel pin and the red-ringed Gekkoukan Private High School crest, over a white shirt and thin black "
           "ribbon tie."},
  {112, 6, "Bebe, a slim teenage boy with light skin, a neat, rounded blond bowl cut with thick straight bangs "
           "down to his eyebrows, and pale turquoise-blue eyes. Even off duty his love of Japan shows, in a "
           "low-key, cool way: a plain dark charcoal-grey long-sleeved crew-neck T-shirt printed on the chest "
           "with a faint, tone-on-tone grey Eastern dragon, its scaly body coiled and its horned head turned to "
           "one side. The portrait ends at mid-chest."},
  {113, 6, "President Tanaka, a middle-aged man with tanned skin, a long face with lines under his eyes, and "
           "thick, wavy, collar-length dark grey hair swept back and falling loosely over his ears. Narrow, "
           "heavy-lidded grey eyes give him a smug, slippery salesman's air, a man who'd sell you anything at a "
           "markup. He wears a grey-green suit jacket with notched lapels over a crisp white dress shirt with a "
           "pointed collar and a plain dark red tie in a neat knot."},
  {114, 6, "Mutatsu, an elderly, heavyset and completely bald man with tanned, deeply lined skin, a furrowed "
           "brow, large ears, grey eyes and loose skin at his neck. He's a gloriously gruff, hard-living monk, "
           "more barroom brawler than holy man. He wears dark charcoal-grey kimono-style robes crossed over "
           "white under-layers, a broad pale cream band running over one shoulder and knotted low on his chest, "
           "and in one hand, heavy with thick gold rings, he holds a fat brown cigar, its ashy tip lit."},
  {114, 901, "Mutatsu, an elderly, heavyset and completely bald man with tanned, deeply lined skin, a furrowed "
             "brow, large ears, grey eyes and loose skin at his neck. He still has the gruff, weathered presence "
             "of a monk who's seen it all, though calmer and plainer here. He wears dark charcoal-grey "
             "kimono-style robes crossed over white under-layers, with a broad pale cream band over one shoulder "
             "knotted low on his chest. Unlike his usual look, no cigar, hand or ring is in view."},
  {115, 6, "Mamoru, an athletic teenage boy with lightly tanned skin and short, spiky dark grey hair cropped "
           "close at the sides, thick dark eyebrows and grey eyes. He's a friendly, upbeat sporty type, the "
           "easygoing star athlete. He wears a light teal zip-up hoodie with the hood down, a silver zipper, "
           "grey knotted drawstrings and a small dark grey triangle logo, each sleeve striped in black edged "
           "with white and printed with grey triangles."},
  {116, 1, "Nozomi, a heavyset teenage boy with fair skin, a big round face with full, chubby cheeks and a "
           "double chin, short neat black hair with one curl springing up on top and another on his forehead, "
           "droopy grey eyes and large ears. He has a lazy, sulky, self-styled-gourmet air that's oddly "
           "endearing. He wears the school summer uniform: a white collared shirt stretched over his broad "
           "frame, with small white buttons, a breast pocket and a thin dark green-black ribbon tie."},
  {116, 2, "Nozomi, a heavyset teenage boy with fair skin, a big round face with full, chubby cheeks and a "
           "double chin, short neat black hair with one curl springing up on top and another on his forehead, "
           "droopy grey eyes and large ears, a sulky, self-styled gourmet. He wears the school winter uniform: a "
           "charcoal-black blazer with silver-white piping, a green-and-cream lapel pin and the red-ringed "
           "school crest, over a white collared shirt and a thin dark green-black ribbon tie."},
  {117, 6, "Akinari, a thin, pale young man with a narrow face, straight chin-length silver-grey hair with a "
           "side parting and bangs sweeping across his forehead, and grey eyes. He has a fragile, wistful, "
           "poetic air, a sickly dreamer with a quietly haunting good look about him. He wears a loose, "
           "long-sleeved button-up shirt with a pointed collar in bold vertical stripes of slate grey-blue and "
           "pale grey-white, small dark buttons, the top ones undone so it falls open at the neck over his "
           "collarbones."},
  {201, 6, "Igor, a short, hunched, wizened old man who looks like a goblin out of a fairy tale, gleefully "
           "sinister and impossible to forget. The first thing you notice is his nose: extremely long, straight "
           "and pointed, jutting out like a spike. He has pale tan skin, a bald dome of a head with wild, wispy "
           "white hair sticking out at the back and sides, long pointed ears, thick dark curling eyebrows and "
           "huge round bulging white eyes threaded with red veins around tiny pupils. He wears a black suit "
           "jacket with buttoned cuffs over a white shirt and dark tie, and thin white gloves on his long bony "
           "fingers."},
  {202, 6, "Elizabeth, a slender young woman with pale skin and the poised, faintly uncanny air of a doll-like "
           "hostess with mischief glinting behind her manners. She has a short silver bob with straight blunt "
           "bangs, the ends curving in at her jaw, bright yellow eyes and pale pink lips. Her Velvet Room "
           "attendant uniform is all royal blue: a small cap with a short brim and a side button, tilted "
           "jauntily on her head, a fitted sleeveless dress that bares her shoulders, with a tall stand-up "
           "collar and a dark seam down the front, and long gloves above her elbows. On the chest sits a large "
           "round emblem: dark navy and sky-blue rings around a dark centre with a small gold dot."},
  {202, 901, "Elizabeth, a slender young woman with pale skin, a short silver bob with straight blunt bangs, "
             "bright yellow eyes and pale pink lips, now peering out through chunky black glasses with thick, "
             "rounded-square frames: a bookish, playing-the-scholar look worn with pure Elizabeth whimsy. The rest "
             "is her royal-blue Velvet Room uniform: a small brimmed cap with a side button tilted on her head, a "
             "fitted sleeveless dress with bare shoulders and a tall stand-up collar, a large round emblem on the "
             "chest, dark navy and sky-blue rings around a dark centre with a gold dot, and long gloves above her "
             "elbows."},
  {202, 902, "Elizabeth, a slender young woman with pale skin, a short silver bob with straight blunt bangs and "
             "bright yellow eyes, playing dress-up as a cat with complete deadpan commitment: fluffy white cat "
             "ears with pink insides stand up on either side of her head, and a white furry cat-muzzle mask with a "
             "pink nose and black whiskers covers her nose and upper lip. Her small tilted cap stays on, and below "
             "is her royal-blue Velvet Room uniform: a fitted sleeveless dress with bare shoulders, a tall "
             "stand-up collar, a large round emblem of dark navy and sky-blue rings around a dark centre with a "
             "gold dot, and long gloves above her elbows."},
  {202, 903, "Elizabeth, a slender young woman with pale skin and a short silver bob with straight blunt bangs, "
             "gleefully disguised in joke novelty glasses: round black frames with white lenses painted in "
             "swirling circles that hide her eyes, a big pinkish-red plastic nose and a bushy black moustache "
             "curled up at the ends. It is a ridiculous gag worn with total elegance. Her small tilted cap stays "
             "on, over her royal-blue Velvet Room uniform: a fitted sleeveless dress with bare shoulders, a tall "
             "stand-up collar, a large round emblem of dark navy and sky-blue rings around a dark centre with a "
             "gold dot, and long gloves above her elbows."},
  {202, 904, "Elizabeth, a slender young woman with pale skin, a short silver bob with straight blunt bangs and "
             "bright yellow eyes, merrily playing Santa: in place of her cap sits a red Santa hat with a thick "
             "white fur brim, its tip flopping to one side into a fluffy white pom-pom, and a fluffy white false "
             "beard and curled moustache hide her mouth and chin. The effect is festive, absurd and oddly "
             "dignified. Below is her royal-blue Velvet Room uniform: a fitted sleeveless dress with bare "
             "shoulders, a tall stand-up collar, a large round emblem of dark navy and sky-blue rings around a "
             "dark centre with a gold dot, and long gloves above her elbows."},
  {211, 6, "Takaya, a gaunt, lean man with the languid menace of a wandering prophet gone wrong, half messiah, "
           "half drifter. He has pale skin, sunken cheeks, stubble on his jaw and chin and unsettling yellow "
           "eyes. His very long, wavy silver-grey hair, parted in the middle, falls in loose waves past his "
           "shoulders and down his chest, held by a thin dark metal circlet across his forehead. He is "
           "bare-chested, purple tattoos on both shoulders: on his right, a crowned serpent coiled around a "
           "spiky sun with a pink spiral at its centre; on his left, a tangle of purple and pale figures mostly "
           "hidden by his hair. The portrait ends at mid-chest."},
  {212, 6, "Jin, a lean young man with pale skin and the guarded, techy look of a back-room hacker who trusts "
           "nobody. His straight, glossy dark blue hair is cut in a jaw-length bob, its long fringe falling over "
           "his left eye, and he wears glasses with black rectangular frames and orange-tinted lenses. His "
           "bright green jacket is zipped right up into a tall stand collar that hides his chin, with two light "
           "grey stripes down the front beside the zip. Black buckled harness straps run over both shoulders, "
           "and a black band is wrapped around his left upper arm. The portrait ends at his upper chest."},
  {213, 6, "Chidori, a pale young woman with the eerie, fragile beauty of a gothic porcelain doll, distant and a "
           "little dangerous. Very long, straight red hair with blunt bangs falls down her back, white ribbons "
           "laced through its strands. She has grey eyes with spiky dark lashes and soft purple eyeshadow. On "
           "her head is a frilly white ruffled headdress with a black band studded with silver diamonds, pierced "
           "through by a large silver dagger-shaped pin with a small charm dangling from its hilt. She wears a "
           "frilly white gothic-lolita dress with ruffled trim and puffed sleeves, and a high ruffled collar "
           "tied with a ribbon bow at her throat. The portrait ends at her chest."},
  {213, 201, "Chidori, a slender, pale young woman stripped of all her gothic finery, which leaves her looking "
             "quiet, fragile and strangely softer. Very long, straight red hair with blunt bangs cut straight "
             "across her brows falls loose past her shoulders and down her back. She has grey eyes rimmed with "
             "spiky dark lashes, soft purple eyeshadow and pale pink lips. No headdress, no ribbons, no jewellery: "
             "just a plain pale aqua-blue hospital gown with a wrap-over front crossing at her chest and loose "
             "sleeves, without any pattern or trim. The portrait ends at her chest."},
  {221, 2, "Ryoji, a slender teenage boy with fair skin and the easy, breezy charm of a smooth transfer student "
           "who knows exactly how to win a room. His dark grey-black hair is swept back from his forehead, a few "
           "strands sticking up on top and the ends flicking out at his nape. He has blue eyes and a small black "
           "beauty mark under his left eye. He wears a white button-front shirt with small dark square buttons, "
           "the dark edges of his open school winter jacket running down either side, and his signature long, "
           "soft pale-yellow scarf, edged in a darker outline, wrapped high around his neck and hanging down the "
           "front. The portrait ends at his chest."},
  {221, 154, "Ryoji, a slender teenage boy with fair skin, dark grey-black hair swept back from his forehead, a "
             "few strands sticking up on top and the ends flicking out at his nape, blue eyes and a small black "
             "beauty mark under his left eye. He keeps his relaxed, breezy charm even here. He is bare-chested and "
             "bare-shouldered, as at a bath or hot spring, with no clothing or jewellery at all, not even his "
             "usual yellow scarf. The portrait ends at his upper chest."},
  {221, 155, "Ryoji, a slender teenage boy with fair skin, looking right at home and pleasantly unwound in an "
             "inn-style yukata. His dark grey-black hair is swept back from his forehead, a few strands sticking "
             "up on top and the ends flicking out at his nape, and he has blue eyes and a small black beauty mark "
             "under his left eye. The yukata is a plain slate grey-blue outer robe with wide sleeves, wrapped over "
             "a white inner robe whose edged collar shows in a V at his throat, patterned with grey chains of "
             "interlinked circles. No scarf or jewellery. The portrait ends at his chest."},
  {222, 6, "Ikutsuki, a lanky middle-aged man with the mild, rumpled look of a harmless academic, the sort of "
           "gentle professor who can't resist a bad pun. He has pale skin and long, wavy brown hair combed back "
           "from a high, receding hairline and falling loose behind his shoulders, grey eyes behind thin silver "
           "wire-rimmed glasses with narrow rounded lenses, and a small pointed goatee. He wears a light tan "
           "double-breasted coat with wide notched lapels and pale buttons over a dark grey turtleneck sweater "
           "knitted in vertical ribs, its high collar folded up under his chin. The portrait ends at his chest."},
  {223, 1, "Natsuki, a teenage girl with tanned skin and the trendy, sharp-edged look of a stylish queen bee "
           "with attitude to spare. Her long, wavy ginger-brown hair has straight, blunt bangs, part of it "
           "gathered into a short, spiky ponytail tied high at the back of her head and the rest falling past "
           "her shoulders. She has brown eyes with thick lashes and glossy lips. She wears the school summer "
           "uniform: a white short-sleeved blouse with a wide, pointed collar, a dark grey knitted V-neck "
           "sweater vest with ribbed edges, and a large red bow tied at the collar. The portrait ends at her "
           "chest."},
  {223, 2, "Natsuki, a teenage girl with tanned skin and the trendy, sharp-edged look of a stylish queen bee "
           "with attitude to spare. Her long, wavy ginger-brown hair has straight, blunt bangs, part of it "
           "gathered into a short, spiky ponytail tied high at the back of her head and the rest falling past "
           "her shoulders. She has brown eyes with thick lashes and glossy lips. She wears the school winter "
           "uniform: a dark grey knitted V-neck cardigan with long sleeves and ribbed edges over a white blouse "
           "with a wide, pointed collar, and a large red bow tied at the collar. The portrait ends at her chest."},
  {223, 6, "Natsuki, a teenage girl with tanned skin, dressed down into something cosy and casual that softens "
           "her usual sharp edge. Her ginger-brown hair has straight, blunt bangs and a short, spiky ponytail "
           "tied high at the back, the rest hanging just to her shoulders. She has brown eyes with thick lashes "
           "and glossy lips. She wears an oversized cream hooded jacket, hanging open with its hood bunched "
           "behind her neck, its lining grey-blue and a round snap button on each edge of the front, over a pink "
           "scoop-neck top, with a small silver heart-shaped pendant on a thin chain at her collarbone. The "
           "portrait ends at her chest."},
  {224, 6, "Takeharu, a stern middle-aged man with the cold, commanding presence of a corporate patriarch, the "
           "eyepatch lending him a hard, almost military edge. He has pale skin and short dark grey hair swept "
           "across his forehead to one side. A black eyepatch covers his right eye, its thin black strap running "
           "diagonally up across his forehead and into his hair; his other eye is grey. He wears an impeccable "
           "grey three-piece suit: a jacket with notched lapels, a darker charcoal waistcoat, a white shirt with "
           "a pointed collar and a dark red tie, with a small round gold pin on his left lapel. The portrait "
           "ends at his chest."},
  {252, 1, "Akihiko as a young boy, a scrappy little tough guy already wearing the marks of picking fights he "
           "means to win. He has fair skin, short, tousled silver-white hair, grey eyes and a beige sticking "
           "plaster across the bridge of his nose. He wears a white collared button-up shirt, unbuttoned at the "
           "neck, with a breast pocket on the left, over a plain red T-shirt whose round neck shows at his "
           "throat. He wears no jewellery. The portrait ends at his chest."},
  {253, 1, "Mitsuru as a younger teenager, already every inch the composed young heiress: neat, ladylike and a "
           "little aloof. She has fair skin and wavy red hair falling in loose curls past her shoulders, with a "
           "long side-swept fringe and a thin purple hairband. She has red eyes and soft pink lips. She wears a "
           "school uniform: a white long-sleeved blouse with a small pointed collar, a dark grey knitted sweater "
           "vest with ribbed edges, and a large red bow at the collar with long, layered ruffled tails. She "
           "wears no jewellery. The portrait ends at her chest."},
  {254, 5, "Mitsuru as a little girl, a prim, perfectly turned-out little heiress who looks dressed for a "
           "portrait sitting. She has fair skin, red eyes and red hair with a side-swept fringe, the rest worn "
           "in long, springy ringlet drill curls hanging on both sides of her head. She wears a white "
           "long-sleeved blouse with fine vertical pleats down the front, small round white buttons and a high "
           "stand collar. A large red bow is tied at the collar, fastened at its centre by a gold-framed oval "
           "brooch set with a dark green-black stone. The portrait ends at her chest."},
  {1081, 6, "Mr. Edogawa, a lanky man with the scruffy, slightly unhinged look of an eccentric who forgot to go "
            "home from the lab last night. He has pale skin, messy, spiky dark hair, thick black rectangular "
            "glasses and heavy stubble on his chin and jaw, with a red pencil tucked behind his left ear. He "
            "wears a rumpled white lab coat, open over a mustard-yellow T-shirt with a dark brown round collar "
            "and a white outline print of a hand with two fingers raised. He wears no jewellery. The portrait "
            "ends at his chest."},
  {1082, 6, "Mr. Ekoda, a middle-aged man with the tired, mild look of a long-suffering salaryman who has sat "
            "through a great many meetings. He has fair skin, lined cheeks and neat, straight grey-brown hair cut "
            "in a mop falling to his brows and over his ears, with grey eyes. He wears a plain grey suit jacket "
            "with notched lapels, a white shirt with a pointed collar, and a tie striped diagonally in purple and "
            "dark grey. He wears no glasses or jewellery. The portrait ends at his chest."},
  {1083, 6, "Mr. Ono, a middle-aged man who is gloriously, unapologetically odd: a history buff in a business "
            "suit wearing a full samurai helmet as if it were perfectly normal. He has tanned skin, brown eyes "
            "and a short goatee. The dark iron helmet is a domed bowl with a tall, curved gold crescent crest "
            "rising from the front, gold-tipped side flaps folded back by his temples and a flared neck guard of "
            "riveted plates at the back, tied under his chin by a dark cord. Below it he wears a brown plaid suit "
            "jacket checked with darker lines, a white shirt with a pointed collar and a red tie. The portrait "
            "ends at his chest."},
  {1084, 6, "Mr. Takenozuka, a man in his thirties with a laid-back, slightly rumpled good look, the easygoing "
            "sort who never quite finished shaving. He has tanned skin, grey eyes and tousled, dark grey hair "
            "parted in the middle and falling over his ears, with light stubble along his jaw and chin. He wears "
            "a plain dark grey suit jacket with notched lapels, a white shirt with a pointed collar, and a red "
            "tie with a pattern of dark diamond checks. He wears no glasses or jewellery. The portrait ends at "
            "his chest."},
  {1085, 6, "Mr. Miyahara, a slight man and a lovable dork from top to toe, with a huge, round dark grey afro "
            "that dwarfs the rest of him. He has pale skin, grey eyes, small round copper wire-rimmed glasses and "
            "one large front tooth showing like a cheerful rabbit's. He wears a mustard-yellow V-neck sweater "
            "over a green shirt with fine darker green pinstripes and a tall, upturned collar. He carries a large "
            "brown wooden set-square, a triangular ruler marked with measuring lines, up over his shoulder like a "
            "prized weapon. The portrait ends at his chest."},
  {1086, 6, "Officer Kurosawa, a hard-faced, no-nonsense man whose cap is pulled low enough to throw his eyes "
            "into shadow, all tough beat-cop gravity. He has tanned skin and short dark hair under a navy peaked "
            "police cap with a glossy black peak and band, a small silver button at the side, and a gold badge on "
            "the front: a star in a wreath of laurel leaves inside a dark circle. He wears a navy uniform jacket "
            "with silver shoulder buttons, a white shirt and a dark navy tie, a gold badge on his chest and a "
            "grey radio microphone clipped at his left shoulder, its coiled cord hanging down. The portrait ends "
            "at his chest."},
  {1087, 6, "An old man with the weathered, gruff dignity of a veteran cook who has run the same kitchen for "
            "decades. He has tanned, deeply wrinkled skin, grey eyes and long blue-grey hair combed back from a "
            "receding hairline and tied in a bushy ponytail at the back of his head, with thick blue-grey "
            "eyebrows and a full, shaggy blue-grey beard and moustache. He wears a white high-collared jacket "
            "with a stand collar and round brass buttons, like a cook's coat. He wears no jewellery. The portrait "
            "ends at his upper chest."},
  {1181, 6, "The principal, a heavyset older man, round, jolly and grandfatherly, the very picture of a "
            "self-satisfied old school official. He has fair skin, grey hair combed back from his forehead and "
            "over his ears, a round face with a double chin and a bushy grey moustache, and small round silver "
            "wire-rimmed glasses with red tips on the earpieces. He wears a dark grey suit jacket over a beige "
            "knitted V-neck sweater vest with ribbed edges, a white shirt with a pointed collar and a red tie. "
            "The portrait ends at his chest."},
  {1581, 6, "Ms. Toriumi, a young woman with a crisp, capable, slightly world-weary look, a professional who "
            "keeps it neat and gets on with it. She has fair skin and a chin-length, wavy light-brown bob with "
            "side-swept bangs, its ends flicking out. She has brown eyes, soft pink lipstick and a small pearl "
            "stud earring in her left ear. She wears a dusty rose suit jacket with notched lapels and a flap "
            "pocket, over a white blouse with a high stand collar and small round white buttons down the front. "
            "The portrait ends at her chest."},
  {1582, 6, "Ms. Ounishi, a woman with a chic, polished look, smart and self-assured with a touch of glamour. Her "
            "brown hair is swept to one side in a long fringe and pinned up into a bun at the back, and she has "
            "brown eyes behind bold red oval-rectangular glasses and pink lips. A necklace of pale pearls rests "
            "at the base of her throat. She wears a white jacket with a wide, open collar and notched lapels over "
            "a plain lilac top with a low, straight neckline that shows a hint of cleavage. She wears no "
            "earrings. The portrait ends at her chest."},
  {1583, 6, "Mrs. Terauchi, a woman with a bright, bubbly, sunny-retro charm, like a cheerful mum stepped out of "
            "a sixties magazine. She has fair skin and shoulder-length brown hair with side-swept bangs, flipped "
            "out at the ends and held back by a yellow hairband. She has brown eyes and pink lipstick. She wears "
            "an orange suit jacket with notched lapels and round buttons over a white top, with a pale yellow "
            "silk scarf tied loosely around her throat, its ends fluttering to one side. The portrait ends at her "
            "chest."},
  {1586, 6, "A woman with a sly, knowing, quietly sultry air, the kind who peers over her glasses as if she "
            "already knows your secrets. She has fair skin and long, wavy black hair swept back from her forehead "
            "and gathered loosely behind her, a few strands curling down by her face, with green eyes and peach "
            "lipstick. Thin, metal-framed rectangular glasses sit perched low on her nose. Her figure-hugging "
            "white long-sleeved top is Chinese in style, with a high mandarin collar and a diagonal front opening "
            "both piped in green, and a thin gold ring necklace, a rigid hoop, sits loosely around her neck. The "
            "portrait ends at her chest."},
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
