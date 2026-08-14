# AdaptiveGate-VST

## Architektúra: sávonkénti detektálás, globális kapu

A gate **egyetlen, globális gain-t** alkalmaz a teljes (nem frekvenciára bontott) jelre - nem kapuz sávonként külön-külön. A frekvenciasávokra bontás kizárólag a *detektálást* szolgálja: minden sávnak saját envelope-követője és zajszint-becslője van, hogy eldönthető legyen, az adott sávban van-e a zajszint fölötti, valós jelre utaló energia. Minden mintavételnél az a sáv "vezényli" a globális döntést, amelyik a legerősebb bizonyítékot adja (SNR a saját margin fölött a legnagyobb), és a globális gain-burkológörbe is az ő attack/hold/release idejét veszi át arra a pillanatra. Ez azért fontos, mert ha minden sáv önállóan nyitna/zárna, a spektrum különböző részei más-más pillanatban kapcsolnának ki-be, ami szűrt, "lyukacsos" hangzást eredményezne - ehelyett a teljes jel egyben, koherensen nyit és zár, csak az, hogy *mikor* nyisson, azt a legmeggyőzőbb frekvenciasáv dönti el.

## Presetek

A vezérlők felett egy preset-sáv teszi lehetővé az összes beállítás (Source-tól Bypass-ig) egy néven történő elmentését és később visszatöltését:

- **Save...** - névre kéri az aktuális beállításokat, majd elmenti egy XML fájlba
- legördülő lista - kiválasztva egy mentett presetet, azonnal betölti
- **Delete** - törli a legördülőben kiválasztott presetet

A presetek a `~/Library/Application Support/Moth Production/AdaptiveGate/Presets` mappában tárolódnak, egy-egy XML fájlként presetenként.

## Kezelőszervek

A plugin jelenleg JUCE beépített generikus paraméterlistája helyett egy kézzel épített, funkcionálisan azonos vezérlőlistát használ felületként (egyedi UI/skin még nincs), kiegészítve egy sávonkénti állapotkövető vizualizációval. Minden vezérlő fölé húzva az egeret egy rövid hint (tooltip) jelenik meg, ami elmagyarázza, mire való - az alábbi szövegek ugyanazok, forráskód szerinti pontos érték-tartományokkal:

**Source** (Speech / Guitar / Distorted Bass / Drum (Close Mic) / Drum (Overhead))
Kiválasztja a forrásanyaghoz illesztett frekvenciasáv-profilt: ez állítja be a crossover pontokat, az egyes sávok detektálási margin-ját, valamint az attack/hold/release és sigmoid-meredekség értékeket, amiket az adott sáv akkor "hoz be" a globális döntésbe, amikor ő vezényel. Alapjaiban meghatározza, mely frekvenciatartományok mennyire könnyen tudják megnyitni a kaput.

**Threshold** (-24 … +24 dB, alapérték 0 dB)
Az összes sáv detektálási küszöbéhez (margin) hozzáadott eltolás. Pozitív érték: a kapu szigorúbb lesz, csak erősebb jel/zaj arány esetén nyit ki (több lesz elnyomva). Negatív érték: engedékenyebb, könnyebben nyit ki (kevesebbet nyom el).

**Sensitivity** (0.1 … 4.0, alapérték 1.0)
A globális sigmoid döntési görbe meredekségét szorozza. Magasabb érték: a kapu élesebben, kapcsolószerűbben vált nyitott/zárt állapot között a küszöb körül. Alacsonyabb érték: fokozatosabb, lágyabb átmenet nyitás és zárás között.

**Range** (0.0 … 1.0, alapérték 0.0)
A globális kapu lezárt állapotú minimális erősítésének padlóértéke, lineáris gain-ben (nem dB). 0.0 = teljes elnémítás záráskor. Magasabb érték (pl. 0.1 ≈ -20 dB) padlót ad, amely alá a kapu sosem csökken - minél nagyobb az érték, annál kevésbé teljes a hatása. 1.0-nál a kapu gyakorlatilag hatástalanná válik (sosem csökkenti az erősítést).

**Attack** (0.1x … 4.0x szorzó, alapérték 1.0x)
Szorzó a mindenkori vezénylő sáv alap attack idejére. Nagyobb érték: lassabban nyit ki a kapu jel megjelenésekor. Kisebb érték: gyorsabban nyit.

**Hold** (0.1x … 4.0x szorzó, alapérték 1.0x)
Szorzó a mindenkori vezénylő sáv alap hold idejére: mennyi ideig marad nyitva a kapu a csúcsérték után, mielőtt megkezdődne a zárás. Nagyobb érték: tovább tartja nyitva.

**Release** (0.1x … 4.0x szorzó, alapérték 1.0x)
Szorzó a mindenkori vezénylő sáv alap release idejére: milyen gyorsan zár be a kapu a hold szakasz után. Nagyobb érték: lassabb, fokozatosabb elhalkulás. Kisebb érték: gyorsabb, hirtelenebb záródás.

**Hysteresis** (0 … 12 dB, alapérték 2.0 dB)
Extra dB-puffer, amit a vezénylő sáv SNR-excess-ének át kell lépnie ahhoz, hogy a (globális) kapu állapotot váltson, ha már egyszer abban az állapotban van (Schmitt-trigger jellegű viselkedés, a kattogás elkerülésére). Magasabb érték: stabilabb, kevésbé csapkod határeseti jelnél, de kevésbé reagál finom változásokra. Alacsonyabb érték: érzékenyebb, de hajlamosabb lehet a villogásra.

**Mix** (0.0 … 1.0, alapérték 1.0)
Dry/Wet keverés a feldolgozatlan és a kapuzott jel között. 0.0 = teljesen dry (a gate hatása nem hallatszik), 1.0 = teljesen wet (a gate teljes hatással érvényesül). Köztes érték párhuzamos ("parallel") gate-eléshez használható.

**Bypass** (be/ki, alapból ki)
Teljesen kikapcsolja a feldolgozást - a jel változtatás nélkül folyik át a pluginon.
