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
bool parseWeight(const QString &text, int *grams, QString *error = nullptr);
bool parseDate(const QString &text, QDate *date, QString *error = nullptr);
QString displayWeight(int grams);
std::optional<int> changeAt(const QList<Entry> &ascending, int days,
                            int window = 3);
QList<Point> points(const QList<Entry> &ascending);
QString csvExport(const QList<Entry> &ascending);
bool csvParse(const QString &text, QList<Entry> *entries, QString *error);
} // namespace pw
