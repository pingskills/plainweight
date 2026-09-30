#include "database/store.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <filesystem>

namespace pw {
QString databasePath() {
  return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
      .filePath(QStringLiteral("plainweight.db"));
}
Store::Store() : m_name(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}
Store::~Store() { close(); }
bool Store::fail(const QString &message) {
  m_error = message;
  return false;
}
bool Store::failOpen(OpenError kind, const QString &message) {
  m_openError = kind;
  return fail(message);
}
const QList<Migration> &migrations() {
  static const QList<Migration> list = {
      {1,
       {QStringLiteral(
            "CREATE TABLE measurements (entry_date TEXT PRIMARY KEY NOT NULL "
            "CHECK(entry_date GLOB "
            "'[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'), weight_grams "
            "INTEGER NOT NULL CHECK(weight_grams BETWEEN 1000 AND 500000), "
            "created_at TEXT NOT NULL, updated_at TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE settings (key TEXT PRIMARY KEY NOT NULL, "
                       "value TEXT NOT NULL)"),
        QStringLiteral("PRAGMA application_id = %1").arg(Store::ApplicationId)}},
  };
  return list;
}
void Store::close() {
  if (!m_db.isValid())
    return;
  m_db.close();
  m_db = QSqlDatabase();
  QSqlDatabase::removeDatabase(m_name);
}
bool Store::run(const QString &sql) {
  QSqlQuery q(m_db);
  if (!q.exec(sql))
    return fail(q.lastError().text());
  return true;
}
bool Store::open(const QString &path, bool readOnly) {
  return open(path, readOnly, migrations());
}
bool Store::open(const QString &path, bool readOnly,
                 const QList<Migration> &steps) {
  close();
  m_error.clear();
  m_openError = OpenError::None;
  const int latest = steps.isEmpty() ? 0 : steps.last().version;
  m_path = path;
  if (readOnly && !QFileInfo::exists(path))
    return failOpen(OpenError::Missing,
                    QStringLiteral("No measurements recorded."));
  const bool newDb = !QFileInfo::exists(path);
  const QString dataDir = QFileInfo(path).absolutePath();
  const bool newDir = !QFileInfo::exists(dataDir);
  if (!readOnly && !QDir().mkpath(dataDir))
    return failOpen(OpenError::Io,
                    QStringLiteral("Cannot create data directory."));
  if (!readOnly && newDir)
    QFile::setPermissions(dataDir, QFileDevice::ReadOwner |
                                       QFileDevice::WriteOwner |
                                       QFileDevice::ExeOwner);
  m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
  m_db.setDatabaseName(path);
  m_db.setConnectOptions(
      readOnly
          ? QStringLiteral("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=5000")
          : QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
  if (!m_db.open())
    return failOpen(OpenError::Io, m_db.lastError().text());
  QSqlQuery q(m_db);
  if (!q.exec(QStringLiteral("PRAGMA application_id")) || !q.next())
    return failOpen(OpenError::Unreadable,
                    QStringLiteral("Not a readable SQLite database."));
  int appId = q.value(0).toInt();
  if (!q.exec(QStringLiteral("PRAGMA user_version")) || !q.next())
    return failOpen(OpenError::Unreadable,
                    QStringLiteral("Cannot read schema version."));
  int version = q.value(0).toInt();
  if (!q.exec(QStringLiteral(
          "SELECT count(*) FROM sqlite_master WHERE type='table'")) ||
      !q.next())
    return failOpen(OpenError::Unreadable,
                    QStringLiteral("Cannot inspect database."));
  int tables = q.value(0).toInt();
  q.finish();
  if ((appId != 0 && appId != ApplicationId) || (appId == 0 && tables != 0))
    return failOpen(OpenError::Foreign,
                    QStringLiteral("Not a PlainWeight database."));
  if (version > latest)
    return failOpen(
        OpenError::Newer,
        QStringLiteral("Database was made by a newer PlainWeight version."));
  if (readOnly)
    return version == 0
               ? failOpen(OpenError::Damaged,
                          QStringLiteral("Database has no measurements schema."))
               : (validateSchema() ||
                  failOpen(OpenError::Damaged, m_error));
  if (!migrate(steps))
    return failOpen(OpenError::Damaged, m_error);
  if (!validateSchema())
    return failOpen(OpenError::Damaged, m_error);
  if (newDb)
    QFile::setPermissions(path,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  return true;
}
bool Store::validateSchema() {
  QSqlQuery q(m_db);
  if (!q.exec(
          QStringLiteral("SELECT entry_date,weight_grams,created_at,updated_at "
                         "FROM measurements LIMIT 0")) ||
      !q.exec(QStringLiteral("SELECT key,value FROM settings LIMIT 0")))
    return fail(QStringLiteral(
        "PlainWeight database schema is incomplete or damaged."));
  return true;
}
bool Store::migrate(const QList<Migration> &steps) {
  QSqlQuery q(m_db);
  if (!q.exec(QStringLiteral("PRAGMA user_version")) || !q.next())
    return fail(QStringLiteral("Cannot read schema version."));
  int version = q.value(0).toInt();
  q.finish();
  // Each step and its version bump commit together; see migrations().
  for (const auto &step : steps) {
    if (step.version <= version)
      continue;
    if (!m_db.transaction())
      return fail(m_db.lastError().text());
    for (const auto &sql : step.statements)
      if (!run(sql)) {
        m_db.rollback();
        return false;
      }
    if (!run(QStringLiteral("PRAGMA user_version = %1").arg(step.version))) {
      m_db.rollback();
      return false;
    }
    if (!m_db.commit()) {
      m_db.rollback();
      return fail(m_db.lastError().text());
    }
    version = step.version;
  }
  return true;
}
QList<Entry> Store::entries() const {
  QList<Entry> list;
  QSqlQuery q(m_db);
  if (!q.exec(
          QStringLiteral("SELECT entry_date,weight_grams,created_at,updated_at "
                         "FROM measurements ORDER BY entry_date")))
    return list;
  while (q.next())
    list << Entry{QDate::fromString(q.value(0).toString(), Qt::ISODate),
                  q.value(1).toInt(), q.value(2).toString(),
                  q.value(3).toString()};
  return list;
}
bool Store::save(const QDate &date, int grams, bool update) {
  if (!date.isValid() || date > today() || grams < 1000 || grams > 500000)
    return fail(QStringLiteral("Invalid date or weight."));
  QSqlQuery q(m_db);
  const QString now =
      QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
  q.prepare(
      update ? QStringLiteral("UPDATE measurements SET "
                              "weight_grams=?,updated_at=? WHERE entry_date=?")
             : QStringLiteral("INSERT INTO "
                              "measurements(entry_date,weight_grams,created_at,"
                              "updated_at) VALUES(?,?,?,?)"));
  if (update) {
    q.addBindValue(grams);
    q.addBindValue(now);
    q.addBindValue(date.toString(Qt::ISODate));
  } else {
    q.addBindValue(date.toString(Qt::ISODate));
    q.addBindValue(grams);
    q.addBindValue(now);
    q.addBindValue(now);
  }
  if (!q.exec())
    return fail(q.lastError().text());
  if (update && q.numRowsAffected() != 1)
    return fail(QStringLiteral("Entry no longer exists."));
  return true;
}
bool Store::edit(const QDate &oldDate, const QDate &date, int grams) {
  if (!oldDate.isValid() || !date.isValid() || date > today() ||
      grams < 1000 || grams > 500000)
    return fail(QStringLiteral("Invalid date or weight."));
  if (oldDate != date) {
    QSqlQuery check(m_db);
    check.prepare(
        QStringLiteral("SELECT 1 FROM measurements WHERE entry_date=?"));
    check.addBindValue(date.toString(Qt::ISODate));
    if (!check.exec())
      return fail(check.lastError().text());
    if (check.next())
      return fail(
          QStringLiteral("A measurement already exists for that date."));
  }
  QSqlQuery q(m_db);
  q.prepare(QStringLiteral(
      "UPDATE measurements SET entry_date=?,weight_grams=?,updated_at=? WHERE "
      "entry_date=?"));
  q.addBindValue(date.toString(Qt::ISODate));
  q.addBindValue(grams);
  q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
  q.addBindValue(oldDate.toString(Qt::ISODate));
  if (!q.exec())
    return fail(q.lastError().text());
  if (q.numRowsAffected() != 1)
    return fail(QStringLiteral("Entry no longer exists."));
  return true;
}
bool Store::remove(const QDate &date) {
  QSqlQuery q(m_db);
  q.prepare(QStringLiteral("DELETE FROM measurements WHERE entry_date=?"));
  q.addBindValue(date.toString(Qt::ISODate));
  return q.exec() ? true : fail(q.lastError().text());
}
std::optional<int> Store::target() const {
  QSqlQuery q(m_db);
  q.prepare(
      QStringLiteral("SELECT value FROM settings WHERE key='target_grams'"));
  if (!q.exec() || !q.next())
    return std::nullopt;
  return q.value(0).toInt();
}
bool Store::setTarget(std::optional<int> grams) {
  QSqlQuery q(m_db);
  if (!grams)
    return run(QStringLiteral("DELETE FROM settings WHERE key='target_grams'"));
  if (*grams < 1000 || *grams > 500000)
    return fail(QStringLiteral("Invalid target weight."));
  q.prepare(
      QStringLiteral("INSERT INTO settings(key,value) VALUES('target_grams',?) "
                     "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
  q.addBindValue(QString::number(*grams));
  return q.exec() ? true : fail(q.lastError().text());
}
bool Store::exportCsv(const QString &path) {
  QSaveFile f(path);
  if (!f.open(QIODevice::WriteOnly))
    return fail(f.errorString());
  auto data = csvExport(entries()).toUtf8();
  if (f.write(data) != data.size() || !f.commit())
    return fail(f.errorString());
  return true;
}
bool Store::importCsv(const QString &path, int *added, int *skipped) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return fail(f.errorString());
  QList<Entry> incoming;
  QString parseError;
  if (!csvParse(QString::fromUtf8(f.readAll()), &incoming, &parseError,
                today()))
    return fail(parseError);
  if (!m_db.transaction())
    return fail(m_db.lastError().text());
  int a = 0, s = 0;
  QSqlQuery q(m_db);
  for (const auto &e : incoming) {
    q.prepare(QStringLiteral(
        "SELECT weight_grams FROM measurements WHERE entry_date=?"));
    q.addBindValue(e.date.toString(Qt::ISODate));
    if (!q.exec()) {
      m_db.rollback();
      return fail(q.lastError().text());
    }
    if (q.next()) {
      if (q.value(0).toInt() != e.grams) {
        m_db.rollback();
        return fail(
            QStringLiteral("Conflict on %1; existing data was not changed.")
                .arg(e.date.toString(Qt::ISODate)));
      }
      ++s;
      continue;
    }
    if (!save(e.date, e.grams)) {
      m_db.rollback();
      return false;
    }
    ++a;
  }
  if (!m_db.commit())
    return fail(m_db.lastError().text());
  if (added)
    *added = a;
  if (skipped)
    *skipped = s;
  return true;
}
bool Store::vacuumInto(const QString &path) {
  QSqlQuery q(m_db);
  q.prepare(QStringLiteral("VACUUM INTO ?"));
  q.addBindValue(path);
  return q.exec() ? true : fail(q.lastError().text());
}
bool Store::inspect(const QString &path, int *count, QString *error) {
  Store source;
  if (!source.open(path, true)) {
    if (error)
      *error = source.error();
    return false;
  }
  QSqlQuery q(source.m_db);
  if (!q.exec(QStringLiteral("PRAGMA quick_check")) || !q.next() ||
      q.value(0).toString() != QLatin1String("ok")) {
    if (error)
      *error = QStringLiteral("Integrity check failed.");
    return false;
  }
  if (!q.exec(QStringLiteral("SELECT count(*) FROM measurements")) ||
      !q.next() || source.m_db.lastError().isValid()) {
    if (error)
      *error = QStringLiteral("Missing measurements table.");
    return false;
  }
  if (count)
    *count = q.value(0).toInt();
  return true;
}
bool Store::backup(const QString &path) {
  if (QFileInfo(path).absoluteFilePath() ==
      QFileInfo(m_path).absoluteFilePath())
    return fail(QStringLiteral("Choose a different backup path."));
  QString partial = path + QStringLiteral(".partial");
  if (QFileInfo::exists(partial))
    return fail(
        QStringLiteral("A partial backup already exists at this path."));
  if (!vacuumInto(partial))
    return false;
  QFile::setPermissions(partial,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  int count;
  QString reason;
  if (!inspect(partial, &count, &reason)) {
    QFile::remove(partial);
    return fail(reason);
  }
  std::error_code ec;
  std::filesystem::rename(QFile::encodeName(partial).toStdString(),
                          QFile::encodeName(path).toStdString(), ec);
  if (ec) {
    QFile::remove(partial);
    return fail(QString::fromStdString(ec.message()));
  }
  return true;
}
bool Store::restore(const QString &path, QString *safetyPath) {
  if (QFileInfo(path).absoluteFilePath() ==
      QFileInfo(m_path).absoluteFilePath())
    return fail(QStringLiteral("Choose a backup file, not the live database."));
  int count;
  QString reason;
  if (!inspect(path, &count, &reason))
    return fail(reason);
  const QString stage = m_path + QStringLiteral(".restoring");
  if (QFileInfo::exists(stage))
    return fail(
        QStringLiteral("An unfinished restore file exists; inspect it first."));
  {
    Store source;
    if (!source.open(path, true))
      return fail(source.error());
    if (!source.vacuumInto(stage))
      return fail(source.error());
    QFile::setPermissions(stage,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  }
  {
    Store staged;
    if (!staged.open(stage)) {
      const QString reason2 = staged.error();
      staged.close();
      QFile::remove(stage);
      return fail(QStringLiteral("Could not prepare backup: %1").arg(reason2));
    }
  }
  if (!inspect(stage, &count, &reason)) {
    QFile::remove(stage);
    return fail(reason);
  }
  const QString safety = QFileInfo(m_path).absoluteDir().filePath(
      QStringLiteral("plainweight-pre-restore-%1.db")
          .arg(QDateTime::currentDateTime().toString(
              QStringLiteral("yyyy-MM-dd-HHmmss-zzz"))));
  if (!vacuumInto(safety)) {
    QFile::remove(stage);
    return false;
  }
  QFile::setPermissions(safety,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  if (!inspect(safety, &count, &reason)) {
    QFile::remove(stage);
    return fail(
        QStringLiteral("Safety backup could not be verified: %1").arg(reason));
  }
  close();
  std::error_code ec;
  std::filesystem::rename(QFile::encodeName(stage).toStdString(),
                          QFile::encodeName(m_path).toStdString(), ec);
  if (ec) {
    open(m_path);
    return fail(QString::fromStdString(ec.message()));
  }
  if (!open(m_path)) {
    QString reason2 = error();
    QFile::remove(m_path);
    QFile::copy(safety, m_path);
    open(m_path);
    return fail(
        QStringLiteral("Restore failed: %1. Previous data recovered from %2")
            .arg(reason2, safety));
  }
  if (safetyPath)
    *safetyPath = safety;
  return true;
}
} // namespace pw
