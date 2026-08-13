Ez egy adaptive gate plugin JUCE frameworkben elkészítve. Jól körülhatárolható, modulos felépítésű, így később használható átültetve más pluginben is, mint beépített modul.

Aszimetrikus trackerrel számolj:
ha az envelope lefelé megy, akkor a noise estimate viszonylag gyorsan követi
ha felfelé, akkor a noise estimate nagyon lassan követi. Így a transienteket nem fogja noisenak tekinteni.

Ezek mellett minimum statistics-et gyűjtünk amivel finomítjuk a becslést. Tároljuk az elmúlt néhány másodperc envelopeját és abból keresünk valamilyen alsó percentilist. Például azaz envelope elmúlt X másodpercének 20. percentilise.

Gate decision:
SNR(t)=E(t)−N(t)

A gate kattogása elkerülése érdekében hysteresist használunk. Ez nagyon fontos!

Attack/hold/release:
Nem egy mute switch, hanem generált gain envelope.

Frekvencia függőség:
Ez az adaptive gate több üzemmódot ismer. Beszédhez, dobhoz, basszusgitárhoz, gitárhoz illeszthető frekvenciafüggő adaptivitást kell bevezetni. A zaj ritkán broadband ugyan olyan mértékben. Ezért külön noise estimate kell minden frekvenciasávra, majd:
SNRk​(t)=Ek​(t)−Nk​(t)

(ehhes a serpent tongue pluginból lehetne meríteni zaj becsléseket)

Javaslat az architecturára:
                     ┌── Fast RMS
Input ──► Filterbank ├── Noise percentile tracker
                     ├── SNR estimation
                     └── transient detector
                              │
                              ▼
                       probability
                       of signal
                              │
                              ▼
                   adaptive soft gate
                              │
                   attack / hold / release
                              │
                              ▼
                           Output
Nem bináris döntésfa, hanem:
p=σ(k(SNR−T))
ahol σ sigmoid, utána pedig:
G = G_\min +(1-G_\min)p

Tehát a gate nem kapcsolgat, hanem fokozatosan dönti el, mennyire valószínű, hogy hasznos jel van jelen.

## Suggested Frequency Behavior

| Frequency Range | Typical Content | Adaptive Behavior |
|---|---|---|
| 20–70 Hz | rumble / unnecessary sub | very aggressive |
| 70–120 Hz | palm-mute low-end | medium |
| 120–250 Hz | weight/body | conservative |
| 250 Hz–1 kHz | body / note information | conservative |
| 1–3.5 kHz | attack, presence, riff clarity | very conservative |
| 3.5–6 kHz | pick, fizz, presence | medium |
| 6–10 kHz | amp fizz, hiss | aggressive |
| 10 kHz+ | mostly noise in many high-gain tones | very aggressive |

A useful transition point is around:

```text
6–7 kHz
```

Above this, the detector should increasingly become noise-biased.

Example opening margins:

```text
100 Hz – 4 kHz:
margin = +4…6 dB

4–7 kHz:
margin = +6…8 dB

7–14 kHz:
margin = +10…14 dB
```

The goal is not to cut the upper range permanently. When the guitar is actually sounding, distortion harmonics should naturally allow the higher bands to open.

---

# Distorted Bass Guitar Preset

Distorted bass often contains noticeable hiss and fizz mainly in the upper spectrum.

## Suggested Frequency Behavior

| Frequency Range | Typical Content | Adaptive Behavior |
|---|---|---|
| 20–35 Hz | rumble / useless sub | very aggressive |
| 35–100 Hz | fundamental / weight | very conservative |
| 100–400 Hz | bass body | conservative |
| 400 Hz–2 kHz | note definition / grind | moderately conservative |
| 2–5 kHz | distortion / attack | medium |
| 5–8 kHz | clank / fizz | stronger gating |
| 8–16 kHz | hiss | very aggressive |

Approximate musical interpretation:

```text
30–100 Hz        fundamental / weight
100–400 Hz       body
400 Hz–1.5 kHz   grind / note definition
1.5–4 kHz        distortion / attack
4–8 kHz          clank / fizz
8 kHz+           hiss
```

## Low-Band Controlled High-Band Gate

A strong idea for distorted bass is to let the low/mid range control whether the high-frequency bands are allowed to open.

Estimate bass presence from:

```text
60–400 Hz
+
400 Hz–1.5 kHz
```

Conceptually:

```text
P_bass = f(E_60-400Hz, E_400-1500Hz)
```

Then use `P_bass` to modify the high-band threshold:

```text
bass present:
8–12 kHz threshold decreases

bass absent:
8–12 kHz threshold increases strongly
```

This allows high-frequency distortion harmonics to pass while suppressing standalone amp / pedal hiss during silence.

---

# Drum Preset

Drums require different logic because transient preservation is critical.

A simple slow gate detector is not enough.

Recommended architecture:

```text
transient detector
+
spectral adaptive gate
```

## Suggested Frequency Behavior

| Frequency Range | Typical Content |
|---|---|
| 20–50 Hz | rumble |
| 50–120 Hz | kick fundamental |
| 120–250 Hz | tom / snare body |
| 250–800 Hz | shell tone / boxiness |
| 800 Hz–2.5 kHz | attack / stick |
| 2.5–6 kHz | crack / presence |
| 6–12 kHz | cymbals |
| 12–20 kHz | cymbal air / hiss |

Recommended detector behavior:

```text
50–250 Hz
-> very fast opening for kick / tom transients

2–6 kHz
-> very fast opening for snare / attack

6–16 kHz
-> must not automatically be considered noise
```

## Separate Drum Modes

### Close-Mic Drums

For kick, snare and tom close mics, aggressive high-band gating can be acceptable.

### Drum Bus / Overheads

Do not aggressively gate the high-frequency region because cymbals live there.

A drum-bus or overhead preset should therefore be much more conservative above `6 kHz`.

---

# Source Weighting Example

A simple source-dependent importance matrix can be used.

`1.0` means the band is strongly considered useful signal.

Lower values mean the detector may more easily treat it as noise.

```text
             Speech   Guitar   Bass   Drum
20–60          0.2      0.1     0.2    0.4
60–120         0.5      0.5     1.0    1.0
120–250        0.8      0.9     1.0    0.9
250–500        1.0      1.0     1.0    0.7
500–1.5k       1.0      1.0     1.0    0.8
1.5–4k         1.0      1.0     0.9    1.0
4–8k           0.8      0.7     0.6    1.0
8–16k          0.5      0.3     0.2    0.8
```

The source weight may modify the SNR margin.

Example:

```text
M_k = M_base + C * (1 - W_k)
```

Where:

- `W_k` = source importance weight
- lower `W_k` -> larger required SNR
- larger required SNR -> band opens less easily

---

# Cross-Band Coherence

For distorted guitar and bass, individual bands should not make decisions completely independently.

A better detector checks whether energy is present in multiple musically related bands simultaneously.

Example for distorted guitar / bass:

```text
100–400 Hz
+
400 Hz–1.5 kHz
+
1.5–4 kHz
```

If several of these bands are active together, there is a high probability that a real note is being played.

In this case, higher bands can be allowed to open:

```text
4–8 kHz
8–16 kHz
```

If only:

```text
8–16 kHz
```

contains energy while the lower and middle bands are inactive, the signal is much more likely to be standalone hiss.

This is especially useful for distorted bass.

---

kutatások amik alapján lehet implementálni:

https://github.com/cpuimage/WebRTC_NS/blob/master/noise_suppression.h?utm_source=chatgpt.com

https://ieeexplore.ieee.org/document/1540228?utm_source=chatgpt.com

https://pmc.ncbi.nlm.nih.gov/articles/PMC10599535/?utm_source=chatgpt.com