# PlainWeight

A small native weight log for Linux, and the second Plain Apps application.
Record a measurement in seconds, see your history and trend, and keep your data locally.

PlainWeight uses C++20, Qt 6, QML and SQLite. It needs no account, server or network connection. It follows Omarchy colours when available and falls back to the Qt/system palette elsewhere.

## Build and run

On Arch/Omarchy install `base-devel cmake ninja qt6-base qt6-declarative hicolor-icon-theme`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/plainweight
```

To install from a checkout on any Linux system with Qt 6.5+ and a C++20 compiler:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

The installed desktop entry appears in standard application launchers, including Omarchy. The installation contains only executable and application assets; it never installs or removes personal measurements. For Arch package building see [AUR.md](docs/AUR.md).

## Using it

Type your weight in kilograms and press Enter. The date defaults to today; type an ISO date (`YYYY-MM-DD`) to record an earlier measurement. There is one entry per date. A duplicate prompts for confirmation before updating. Select a history row to edit or delete it; deletion asks for confirmation. The target weight is optional in the Edit menu. The graph can show one, three or six months, one year, or all entries. Toggle its trend line with the checkbox.

Shortcuts: Ctrl+N new entry, Ctrl+S save, Ctrl+E edit selected, Delete delete selected, Ctrl+B backup, Ctrl+Q quit. Tab moves between controls.

The latest recorded weight is shown with its actual date. The previous comparison uses the preceding recorded entry. Seven and thirty day comparisons use the measurement nearest to exactly seven or thirty days before the latest entry, within three or seven calendar days either side respectively. Ties use the older measurement. When none exists, no change is shown. The overall change is latest minus first. All changes are in kg; a positive number means an increase.

The optional seven day trend at a recorded date averages actual measurements from that date and the preceding six calendar days. Missing dates are ignored, never counted as zero. It appears only when at least three measurements fall in the window. The graph joins actual measurements by date, without creating points for missing days. Display numbers round to one decimal place; SQLite stores exact integer grams and CSV uses three decimals.

## Data and backups

The database is `$XDG_DATA_HOME/plainweight/plainweight.db`, ordinarily `~/.local/share/plainweight/plainweight.db`. Each Linux user gets their own database on first GUI launch. No sample data is inserted. Schema version and app identity are stored in SQLite. Future migrations are applied transactionally without resetting measurements.

File → Backup Data writes a consistent SQLite copy to a location you choose. File → Restore Data checks the app identity, schema and integrity before replacing your data; it first saves `plainweight-pre-restore-<timestamp>.db` beside the live database. Keep that safety copy until you verify the restored data. PlainRun databases are rejected.

CSV export writes UTF-8 `date,weight_kg` with ISO dates and three decimal places. Import accepts the same header and format. It validates the complete file, skips exact duplicates, and rejects a date whose existing weight differs. An import is one transaction: any error leaves the database unchanged.

## Read-only CLI

`plainweight --current` prints `YYYY-MM-DD<TAB>weight_kg` with three decimals. `--change-week` and `--change-month` print signed changes in kg with three decimals, using the same comparison rule as the UI. `--version` and `--help` do not open the database. Exit codes are 0 success, 1 data error, 2 invalid command, and 3 no suitable measurement. CLI commands do not open a graphical window or create a database.

See [database documentation](docs/DATABASE.md) and the [Plain Apps guidelines](docs/PLAIN_APPS_GUIDELINES.md).
