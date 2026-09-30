#pragma once
#include "core/weight.h"
#include <QSqlDatabase>

namespace pw {
class Store {
public:
  static constexpr int ApplicationId = 0x506c5767; // PlWg
  static constexpr int SchemaVersion = 1;
  Store();
  ~Store();
  bool open(const QString &path, bool readOnly = false);
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
  bool run(const QString &sql);
  bool migrate();
  bool validateSchema();
  bool vacuumInto(const QString &path);
  QString m_name;
  QString m_path;
  QString m_error;
  QSqlDatabase m_db;
};
QString databasePath();
} // namespace pw
