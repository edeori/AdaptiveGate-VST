# AdaptiveGate-VST

## Kezelőszervek

A plugin jelenleg JUCE beépített generikus paraméterlistáját használja felületként (egyedi UI még nincs). Az alábbi kezelőszervek érhetők el, forráskód szerinti pontos érték-tartományokkal:

**Source** (Speech / Guitar / Distorted Bass / Drum (Close Mic) / Drum (Overhead))
Kiválasztja a forrásanyaghoz illesztett frekvenciasáv-profilt: ez állítja be a crossover pontokat, az egyes sávok alap attack/hold/release idejét, valamint a sávonkénti margin és sigmoid-meredekség értékeket. Alapjaiban meghatározza, mely frekvenciatartományokat mennyire agresszíven kezeli a gate.

**Threshold** (-24 … +24 dB, alapérték 0 dB)
Az összes sáv SNR-küszöbéhez hozzáadott eltolás. Pozitív érték: a gate szigorúbb lesz, csak erősebb jel/zaj arány esetén nyit ki (több lesz elnyomva). Negatív érték: engedékenyebb, könnyebben nyit ki (kevesebbet nyom el).

**Sensitivity** (0.1 … 4.0, alapérték 1.0)
A sigmoid döntési görbe meredekségét szorozza. Magasabb érték: a gate élesebben, kapcsolószerűbben vált nyitott/zárt állapot között a küszöb körül. Alacsonyabb érték: fokozatosabb, lágyabb átmenet nyitás és zárás között.

**Range** (0.0 … 1.0, alapérték 0.0)
A lezárt állapotú minimális erősítés padlóértéke, lineáris gain-ben (nem dB). 0.0 = teljes elnémítás záráskor. Magasabb érték (pl. 0.1 ≈ -20 dB) padlót ad, amely alá a gate sosem csökken - minél nagyobb az érték, annál kevésbé teljes a gate hatása. 1.0-nál a gate gyakorlatilag hatástalanná válik (sosem csökkenti az erősítést).

**Attack** (0.1x … 4.0x szorzó, alapérték 1.0x)
Az aktuális Source-profil alap attack idejét szorozza sávonként. Nagyobb érték: lassabban nyit ki a gate jel megjelenésekor. Kisebb érték: gyorsabban nyit.

**Hold** (0.1x … 4.0x szorzó, alapérték 1.0x)
Az alap hold idő szorzója: mennyi ideig marad nyitva a gate a csúcsérték után, mielőtt megkezdődne a zárás. Nagyobb érték: tovább tartja nyitva a gate-et.

**Release** (0.1x … 4.0x szorzó, alapérték 1.0x)
Az alap release idő szorzója: milyen gyorsan zár be a gate a hold szakasz után. Nagyobb érték: lassabb, fokozatosabb elhalkulás. Kisebb érték: gyorsabb, hirtelenebb záródás.

**Hysteresis** (0 … 12 dB, alapérték 2.0 dB)
Extra dB-puffer, amit az SNR-nek át kell lépnie ahhoz, hogy a gate állapotot váltson, ha már egyszer abban az állapotban van (Schmitt-trigger jellegű viselkedés, a kattogás elkerülésére). Magasabb érték: stabilabb, kevésbé csapkod határeseti jelnél, de kevésbé reagál finom változásokra. Alacsonyabb érték: érzékenyebb, de hajlamosabb lehet a villogásra.

**Mix** (0.0 … 1.0, alapérték 1.0)
Dry/Wet keverés a feldolgozatlan és a kapuzott jel között. 0.0 = teljesen dry (a gate hatása nem hallatszik), 1.0 = teljesen wet (a gate teljes hatással érvényesül). Köztes érték párhuzamos ("parallel") gate-eléshez használható.

**Bypass** (be/ki, alapból ki)
Teljesen kikapcsolja a feldolgozást - a jel változtatás nélkül folyik át a pluginon.
