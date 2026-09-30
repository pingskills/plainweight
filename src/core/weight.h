#pragma once
#include <QDate>
#include <QList>
#include <QString>
#include <optional>

namespace pw {
struct Entry {
  QDate date;
  int grams = 0;
  QString createdAt;
  QString updatedAt;
};
struct Point {
  QDate date;
  double kg = 0;
  std::optional<double> trend;
};
// Accepts "82", "82.4", "82,4" (up to three decimals) between 1 and 500 kg.
bool parseWeight(const QString &text, int *grams, QString *error = nullptr);
// ISO YYYY-MM-DD, not later than `today` (an invalid `today` means the real
// current date; tests pass a fixed date).
bool parseDate(const QString &text, QDate *date, QString *error = nullptr,
               const QDate &today = QDate());
QString displayWeight(int grams);
std::optional<int> changeAt(const QList<Entry> &ascending, int days,
                            int window = 3);
QList<Point> points(const QList<Entry> &ascending);
QString csvExport(const QList<Entry> &ascending);
// Splits one CSV record, honouring RFC 4180 double-quoted fields ("" is a
// literal quote). Returns false for malformed quoting. Records never span lines
// in the PlainWeight format.
bool csvSplit(const QString &line, QStringList *fields);
bool csvParse(const QString &text, QList<Entry> *entries, QString *error,
              const QDate &today = QDate());
} // namespace pw
