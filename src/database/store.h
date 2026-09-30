#pragma once
#include "core/weight.h"
#include <QSqlDatabase>
#include <QStringList>

namespace pw {
// Upgrades the schema from (version - 1) to version. Append-only: never edit a
// released migration.
struct Migration {
  int version = 0;
  QStringList statements;
};
const QList<Migration> &migrations();

class Store {
public:
  static constexpr int ApplicationId = 0x506c5767; // PlWg
  static constexpr int SchemaVersion = 1;
  // Why the last open() failed, so callers never have to compare messages.
  enum class OpenError { None, Missing, Unreadable, Foreign, Newer, Damaged, Io };
  Store();
  ~Store();
  bool open(const QString &path, bool readOnly = false);
  bool open(const QString &path, bool readOnly, const QList<Migration> &steps);
  OpenError openError() const { return m_openError; }
  // Tests pin "today" so future-date checks are deterministic.
  void setFixedToday(const QDate &date) { m_fixedToday = date; }
  QDate today() const {
    return m_fixedToday.isValid() ? m_fixedToday : QDate::currentDate();
  }
  void close();
  QString error() const { return m_error; }
  QString path() const { return m_path; }
  QList<Entry> entries() const;
  bool save(const QDate &date, int grams, bool update = false);
  bool edit(const QDate &oldDate, const QDate &date, int grams);
  bool remove(const QDate &date);
  std::optional<int> target() const;
  bool setTarget(std::optional<int> grams);
  bool exportCsv(const QString &path);
  bool importCsv(const QString &path, int *added = nullptr,
                 int *skipped = nullptr);
  bool backup(const QString &path);
  bool restore(const QString &path, QString *safetyPath);
  static bool inspect(const QString &path, int *count, QString *error);

private:
  bool fail(const QString &message);
  bool failOpen(OpenError kind, const QString &message);
  bool run(const QString &sql);
  bool migrate(const QList<Migration> &steps);
  bool validateSchema();
  bool vacuumInto(const QString &path);
  QString m_name;
  QString m_path;
  QString m_error;
  OpenError m_openError = OpenError::None;
  QDate m_fixedToday;
  QSqlDatabase m_db;
};
QString databasePath();
} // namespace pw
