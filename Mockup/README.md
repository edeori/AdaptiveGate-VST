# Adaptive Gate interactive UI concept

Open `index.html` directly in a modern browser. The prototype has no build step and does not modify the JUCE editor. Append `?shop=1` for a native 1100x720 presentation render without the surrounding preview background or help text.

## Interaction

- Drag a knob vertically; double-click it to restore its default.
- Move Threshold horizontally and watch all per-band decision lines move together.
- Source changes the demo spectrum and band emphasis.
- Demo Signal pauses/resumes the deterministic visual input.
- Advanced swaps the status overview for Attack, Hold, Release and Hysteresis.
- Factory preset navigation applies example UI states. Save adds a local browser-only preset.
- Bypass dims the processing area and leaves the header available.

## Parameter mapping

| UI | APVTS parameter | Display |
| --- | --- | --- |
| Source | `sourceType` | existing five choices |
| Threshold | `threshold` | -24 to +24 dB |
| Sensitivity | `sensitivity` | 0.10 to 4.00 |
| Range | `range` | 0 to 100% |
| Attack | `attack` | 0.10x to 4.00x |
| Hold | `hold` | 0.10x to 4.00x |
| Release | `release` | 0.10x to 4.00x |
| Hysteresis | `hysteresis` | 0 to 12 dB |
| Mix | `mix` | 0 to 100% |
| Bypass | `bypass` | boolean |

The visualizer mirrors the engine model: each band has its own envelope, noise floor and threshold, while the centre shows the single global gain driven by the strongest band.

## Reusable implementation assets

- `assets/adaptive-gate-panel.png` — native 1100x720 empty baked panel shell. It was generated with the built-in image generation workflow from the DeathWings and Metamorphosis background assets as style references.
- `assets/adaptive-gate-symbol.svg` — resolution-independent gate/spectrum emblem for the centre display or empty states.
- `assets/ui-tokens.json` — native dimensions, panel bounds, palette, typography and component sizes for JUCE implementation.
- Shared fonts, logo, button chrome, knob and slider art are referenced from `MothProduction-Shared-VST/UI`, so they stay single-source.

The generated background prompt requested a front-facing dark brushed-metal Moth Production panel with a 78px header, large visualizer well, right control well and bottom control well; cyan/violet edge accents; no text, controls, meters, logo or watermark.

## Native JUCE implementation

The concept is implemented in `Source/PluginEditor.*`, `Source/ControlPanelComponent.*`, `Source/GateVisualizerComponent.*` and `Source/UI/`. `adaptive-gate-native.png` is a direct capture of the normal JUCE Standalone editor. The optional `ADAPTIVEGATE_LAYOUT_TUNER` CMake flag adds the debug-only F2 layout overlay; it defaults to `OFF`.
