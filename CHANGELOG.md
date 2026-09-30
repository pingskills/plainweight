# Changelog

## [0.1.0] - 2026-09-30

- Initial PlainWeight implementation: weight entry, history, graph, statistics, target, backup and restore, CSV and CLI.
- Weights accept a comma decimal (`82,4`); CSV import accepts RFC 4180 quoted fields.
- Only PlainWeight's own CLI options run headless; standard Qt options start the GUI.
- Menus show real keyboard shortcuts and mnemonics; the About dialog shows the build's version.
- Omarchy integration isolated in `src/omarchy/`, honours the theme's `mode`, and keeps secondary text at WCAG AA contrast.
