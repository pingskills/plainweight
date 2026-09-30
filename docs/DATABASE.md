# Database

`QStandardPaths::AppDataLocation` resolves to `$XDG_DATA_HOME/plainweight` or `~/.local/share/plainweight`. The file is `plainweight.db`, private to the current Linux user.

SQLite `application_id` is `0x506c5767` (`PlWg`), and `user_version` is the schema version. Version 1 has:

```sql
CREATE TABLE measurements (
  entry_date TEXT PRIMARY KEY NOT NULL,
  weight_grams INTEGER NOT NULL CHECK(weight_grams BETWEEN 1000 AND 500000),
  created_at TEXT NOT NULL,
  updated_at TEXT NOT NULL
);
CREATE TABLE settings (key TEXT PRIMARY KEY NOT NULL, value TEXT NOT NULL);
```

Dates are ISO `YYYY-MM-DD`; timestamps are UTC ISO 8601 with milliseconds. The only setting is `target_grams`. All statistics and trend values are derived. Migrations increment `user_version` in the same transaction as their schema statements. Foreign or newer databases are refused. Backup files are standalone SQLite databases with the same schema and app ID.
