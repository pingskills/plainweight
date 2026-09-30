# Plain Apps guidelines

The conventions shared by the Plain Apps family live in **one canonical
document**, maintained in the PlainRun repository:

<https://github.com/pingskills/plainrun/blob/main/docs/PLAIN_APPS_GUIDELINES.md>

Each app links to it rather than keeping its own copy, so the family cannot
drift apart. Propose changes to the conventions there.

## PlainWeight specifics

| Item | Value |
|---|---|
| Executable / package | `plainweight` |
| Application ID | `io.github.pingskills.plainweight` |
| SQLite `application_id` | `0x506c5767` (`PlWg`) |
| Data | `$XDG_DATA_HOME/plainweight/plainweight.db` |
| Base unit | integer grams |
| CLI | `--current`, `--change-week`, `--change-month`, `--version`, `--help` |
