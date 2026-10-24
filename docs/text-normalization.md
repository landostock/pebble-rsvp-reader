# Text Normalization

Text fidelity is a release-blocking requirement. The application must preserve the author's spelling, punctuation, and Unicode characters.

## Current Shared Rules

Implemented in [shared/src/unicode.js](../shared/src/unicode.js).

- Decode common named and numeric HTML entities.
- Normalize output to Unicode NFC.
- Convert non-breaking and thin spacing characters to regular spaces for RSVP tokenization.
- Collapse repeated whitespace inside a paragraph.
- Preserve paragraph boundaries before tokenization.
- Remove soft hyphen characters because they are layout hints.
- Reconstruct confident line-break hyphenation:
  - lower-letter + hyphen + newline + lower-letter removes the layout hyphen;
  - letter/digit + hyphen + newline + letter/digit otherwise preserves the lexical hyphen and removes the line break.

## Explicitly Forbidden

- Spelling correction.
- Quote modernization.
- Dash modernization.
- Character translation.
- Replacing unsupported glyphs with `?` silently.

## Mandatory German Sample

```text
Donaudampfschifffahrtsgesellschaft
Straßenverkehrsordnung
größer
äußerst
„Warum?“, fragte er.
Da war’s plötzlich still.
```

The shared tests verify preservation of these characters in tokenization, pacing, and sentence handling. Pebble glyph rendering still requires real hardware verification.

## EPUB Rules For Later Phases

The server EPUB processor must use the EPUB spine as canonical reading order. It must parse `container.xml`, the OPF package document, manifest, spine, navigation/TOC, and XHTML/HTML content. It must ignore scripts, stylesheets as text, and non-reading assets.

The RSVP Nano conversion specification was inspected as the reference behavior for these rules.
