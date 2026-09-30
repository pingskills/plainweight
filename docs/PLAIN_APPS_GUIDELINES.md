# Plain Apps guidelines

Conventions for PlainRun, PlainWeight, and future PlainTasks: small, fast, native Linux applications that each do one thing well. Each has its own repository, package and database. None requires another. A shared library should wait until the third app has demonstrated stable duplicated code.

## Identity and architecture

- Display name `Plain` + noun; lowercase executable, repository and Arch package; app ID `io.github.pingskills.<executable>`.
- Qt application name equals the executable; leave organisation name unset for `$XDG_DATA_HOME/<app>`.
- Use C++20, Qt 6, Qt Quick/QML, Qt SQL, SQLite and CMake. Keep networking, servers, accounts, telemetry and web runtimes out of the application.
- Put calculations and validation in `src/core`, SQLite code in `src/database`, QML-facing logic in `src/services`, UI theme code in `src/ui`, and visual layout in `qml`. Use only as many files as the app needs.
- Link nonvisual code to Qt Core and SQL so it can be tested headlessly. Declare the version once in CMake.

## Data and safety

- Store per-user data in `QStandardPaths::AppDataLocation`, never in the install tree. Packages never create or delete a user's database. Tests use isolated temporary databases.
- Use an app-specific SQLite `application_id`, `user_version` and transactional, append-only migrations. Reject foreign, corrupt and newer databases. Never reset existing data after an open failure.
- Store base units as integers with unit names in columns; dates as ISO text and timestamps as UTC ISO 8601. Derive statistics rather than caching them.
- Backup with SQLite `VACUUM INTO`, verify the copy, and restore only after validation and a safety backup. Failed restores must leave existing data available.
- CSV import validates every row before writing. Explain duplicate policy, and import in a single transaction.
- CLI reads only, never creates the database, uses ISO dates and C-locale numbers, and exits 0 success, 1 data error, 2 usage error or 3 nothing to report.

## Interface

- One obvious primary task on the first screen. Use restrained spacing, one accent colour, tabular numerals, visible focus, plain labels and keyboard shortcuts. No dashboard cards, gamification or decorative animation.
- Follow Qt/system colours, and read active Omarchy theme colours when available. Omarchy is optional; watch theme changes. Respect desktop font sizing and scaling.
- Show empty states honestly. Identify actual observations separately from calculated summaries; document each calculation and missing-data rule.
- Use a virtualized list for unbounded history, a simple Qt Quick chart, and a sensible minimum window size rather than a fixed canvas.

## Desktop and release

- Install with `GNUInstallDirs`: executable, desktop entry, metainfo, SVG and rendered icons, and license. App ID is the desktop file, icon and Wayland app ID.
- Validate desktop metadata, build and test on Arch in CI, then test staged installation. Tag semantic versions `vX.Y.Z`. GitHub's tagged source archive is the package source. Use a real checksum once the tag exists.
- The AUR repository is separate and contains the PKGBUILD and generated `.SRCINFO` only. Never include personal data, backups, credentials, build trees or tokens.

## PlainTasks considerations

PlainTasks should keep the same naming, data, theme, backup, desktop, CLI and release conventions. Its task data will differ from both running and weight measurements; choose a minimal schema for tasks, and do not carry weight-trend or run-statistics concepts into it. Any shared code should be considered only after all three applications show a stable need.

## Recommended PlainRun follow-ups

- Confirm whether its CLI should treat an absent database consistently as exit 3 or app-specific zero totals; document the distinction across the family.
- Check its restore failure path when a live database cannot reopen after file replacement, especially around temporary files and concurrent instances.
- Consider the same one-line CSV conflict explanation in the import dialog if it is not already explicit.

These are notes for future review; PlainRun has not been changed.
