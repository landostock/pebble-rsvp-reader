# Web PWA

Status: Phase 4 local PWA scaffold.

The web app is the primary library-management UI for desktop and Android browsers. It must support authenticated upload, retryable processing, metadata editing, cover replacement, deletion confirmation, progress inspection, and Android companion token pairing.

Implemented now:

- responsive library grid/list;
- EPUB upload through the local API from the Library plus button or drag-and-drop;
- automatic EPUB cover extraction for common OPF metadata forms, EPUB guide cover pages, and existing stored sources;
- title/author search, sort, and grid/list controls confined to the library view;
- book detail metadata editing;
- custom cover upload;
- book deletion from the detail pane;
- progress lookup inside the selected book details;
- diagnostics and storage usage display;
- visible protected-library state for signed-out production users;
- visible sign-in/sign-out control for the production session endpoint, with explicit local dev-mode status;
- named Android device-token generation, one-time pairing-data copy, paired-device listing, and revocation;
- language switch for English and German UI text.

Not implemented yet:

- direct browser-to-Tigris presigned upload;
- watch-sized cover derivation for Pebble transfer;
- QR-code handoff to the Android companion (manual pairing-data copy is implemented);
- offline PWA service worker.
