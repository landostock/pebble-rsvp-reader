# Watch App

The Pebble C app targets Pebble Time 2 (`emery`). It receives the compact library and rolling reader windows from the Android companion, persists book position and reader preferences locally, and keeps three compact 64-token windows available for continuous RSVP playback.

## Controls

Reader:

- Select: play or pause.
- Up / Down: increase or decrease WPM.
- Hold Select: switch directly between RSVP and full text.
- Hold Up: open the reader menu for chapters, library, and settings.
- Back: jump directly to the previous sentence, including across reader-window boundaries.

Full text and lists:

- Up / Down: previous or next text window or list entry.
- Select: activate the selected entry.
- Back: return to the previous screen.

Touch on supported firmware:

- Tap: Select.
- Swipe up / down: WPM or list navigation.
- Swipe left / right: next or previous sentence; in full text, next or previous window.
- A tap in the reader acts like Select; a tap on a visible book or setting row selects and activates that row.

## Reader Profiles

- Focus: while playing, show only the word, red ORP letter, and orientation guides. Paused state restores title, WPM, and progress.
- Informative: keep title, WPM, progress, ORP color, and guides visible.
- Custom: toggle title, WPM, progress, guides, and the ORP marker independently.
- ORP-Farbe is available globally with red, white, yellow, cyan, and green choices. White uses a softer gray for the surrounding letters so the focus remains legible.

Settings also expose WPM, sentence-aware or exact resume, adaptive long-word typography, and touch input. Reader position and settings use separate persistence records so upgrades do not discard progress.

Back navigation is sentence-aware and repeatable without an artificial window limit: each press moves to the preceding sentence start. If that sentence is outside the three prefetched windows, Android resolves it from the complete reader package.

## Build

Build with Pebble SDK 4.33.1 or newer for the official tap and swipe recognizer APIs:

```sh
cd watch
pebble build
```
