# Guitar Hold/Release default — miért változott 1.0x → 0.3x

## Előzmény

A tubescreamer-vst (MothBite Tube Screamer-clipper) projekten dolgozva, valós DI-felvétellel ("Earth Elemental - rythm gtr") mérve kiderült, hogy az AdaptiveGate "Guitar" forrásprofilja a Hold/Release szorzók gyári alapértékén (1.0x/1.0x) **gyakorlatilag nem záródik be** egy valós palm-mute után (800ms-en belül sem), ami szűk ritmusgitár-játéknál hallható zaj-átszűrődést okoz mute-ok/hangok között — ez volt a MothBite-tal együtt észlelt "nem elég gyors" probléma valódi oka. A teljes levezetés (beleértve a módszertani hibát is, ami eredetileg elfedte a problémát): `/Users/mothproduction/Documents/VSCode/tubescreamer-vst/docs/noise-floor-investigation.md`.

## Gyökérok

A gate globális záródási DÖNTÉSE (nem a záródás sebessége) a vezénylő sáv SNR-excess értékétől függ (`excess = envelope − noiseFloor − margin`). Threshold=0dB (gyári alapérték) mellett ez a margin elég bőkezű ahhoz, hogy egy valódi, lecsengő palm-mute jelenergiája **~200-350ms-ig** még "valódi jelnek" számítson — a Hold/Release szorzók csak AZUTÁN számítanak, hogy ez a döntés "zárt"-ra váltott. Ezért a Hold/Release szorzó önmagában, Threshold-módosítás nélkül, csak részleges javulást ad.

## Mérés: 1.0x vs. 0.3x, Threshold=0dB (változatlan)

A gate saját belső gain-értékét mérve (nem a downstream feldolgozott jelből visszakövetkeztetve — ez utóbbi módszertani hiba volt a korábbi vizsgálatban), 3 valódi palm-mute-eseményen:

| Beállítás | c1 (−20dB) | c1 (−40dB) | c2 (−20dB) | c2 (−40dB) | c3 (−20dB) | c3 (−40dB) |
|---|---|---|---|---|---|---|
| RÉGI default (1.0x/1.0x) | soha (800ms alatt) | soha | 347.5ms | soha | 347.5ms | soha |
| ÚJ default (0.3x/0.3x) | soha | soha | **173.4ms** | **254.6ms** | **173.4ms** | **254.6ms** |

Az új 0.3x default a záródási időt kb. **felére csökkenti** ott, ahol a régi egyáltalán be tudott záródni belátható időn belül, és ELÉRI a mélyebb (-40dB) záródást is, amit a régi soha. Az 1. jelöltnél (c1) egyik beállítás sem záródik be 800ms alatt — ez azt mutatja, hogy a Hold/Release-szorzó önmagában NEM old meg minden esetet; a **Threshold** paraméter (élőben állítható, a pedál nézeten is elérhető gomb) marad az erősebb kar azoknál a hangoknál, ahol ez sem elég.

## Változtatás

- `Source/PluginProcessor.cpp::createParameterLayout()`: `hold` és `release` `AudioParameterFloat` gyári alapértéke **1.0f → 0.3f**.
- `Source/ControlPanelComponent.cpp`: a megfelelő csúszkák `setDoubleClickReturnValue()`-ja is 0.3-ra igazítva, hogy dupla-kattintásra ne a régi alapértékre álljon vissza.
- Ez **minden** forrásprofilt érint (nem csak Guitar-t), mivel a szorzó globális, profil-független paraméter — a döntés tudatos: a régi 1.0x default gyakorlatilag "sosem záródik be" viselkedése egyik forrástípusnál sem tűnt hasznos alapértéknek.

## Módszertani megjegyzés

A gate close time-ot a **gate saját belső gain-állapotán** (`AdaptiveGateEngine::getMeterSnapshot()[0].gain`) mérve kell vizsgálni, NEM a downstream feldolgozott (pl. klippelt) jel valamilyen dB-küszöbön való átesésén — ez utóbbi a nyers jel saját dinamikájával keveredik össze, és félrevezető (hamisan "működik"-nek tűnő) eredményt ad, amíg a gate ténylegesen még nyitva van.
