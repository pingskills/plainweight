#include "core/weight.h"
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace pw {
bool parseWeight(const QString &text, int *grams, QString *error) {
  static const QRegularExpression pattern(
      QStringLiteral(R"(^\s*([0-9]{1,3})(?:[.,]([0-9]{1,3}))?\s*$)"));
  auto m = pattern.match(text);
  if (!m.hasMatch()) {
    if (error)
      *error = QStringLiteral(
          "Enter a weight in kg with up to three decimal places.");
    return false;
  }
  const int whole = m.captured(1).toInt();
  const int fraction = m.captured(2).leftJustified(3, QLatin1Char('0')).toInt();
  const int value = whole * 1000 + fraction;
  if (value < 1000 || value > 500000) {
    if (error)
      *error = QStringLiteral("Weight must be between 1 and 500 kg.");
    return false;
  }
  if (grams)
    *grams = value;
  return true;
}
bool parseDate(const QString &text, QDate *date, QString *error,
               const QDate &today) {
  static const QRegularExpression pattern(
      QStringLiteral(R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}$)"));
  const QDate latest = today.isValid() ? today : QDate::currentDate();
  QDate d = QDate::fromString(text, Qt::ISODate);
  if (!pattern.match(text).hasMatch() || !d.isValid() || d > latest) {
    if (error)
      *error = QStringLiteral(
          "Enter a valid date no later than today (YYYY-MM-DD).");
    return false;
  }
  if (date)
    *date = d;
  return true;
}
QString displayWeight(int grams) {
  return QString::number(grams / 1000.0, 'f', 1);
}
std::optional<int> changeAt(const QList<Entry> &entries, int days, int window) {
  if (entries.size() < 2)
    return std::nullopt;
  const Entry &latest = entries.last();
  const QDate target = latest.date.addDays(-days);
  const Entry *best = nullptr;
  int bestDistance = window + 1;
  for (const Entry &e : entries) {
    if (e.date >= latest.date)
      break;
    int distance = std::abs(e.date.daysTo(target));
    if (distance <= window &&
        (distance < bestDistance ||
         (distance == bestDistance && (!best || e.date < best->date)))) {
      best = &e;
      bestDistance = distance;
    }
  }
  if (!best)
    return std::nullopt;
  return latest.grams - best->grams;
}
QList<Point> points(const QList<Entry> &entries) {
  QList<Point> result;
  for (int i = 0; i < entries.size(); ++i) {
    const auto &e = entries[i];
    Point p{e.date, e.grams / 1000.0, std::nullopt};
    // Each plotted trend is the mean of measurements in the inclusive
    // seven-calendar-day window. Missing days are ignored. Require at least
    // three actual measurements, on three dates.
    int sum = 0, count = 0;
    for (int j = i; j >= 0 && entries[j].date >= e.date.addDays(-6); --j) {
      sum += entries[j].grams;
      ++count;
    }
    if (count >= 3)
      p.trend = sum / (1000.0 * count);
    result << p;
  }
  return result;
}
QString csvExport(const QList<Entry> &entries) {
  QString out = QStringLiteral("date,weight_kg\n");
  for (const auto &e : entries)
    out += e.date.toString(Qt::ISODate) + QLatin1Char(',') +
           QString::number(e.grams / 1000.0, 'f', 3) + QLatin1Char('\n');
  return out;
}
bool csvSplit(const QString &line, QStringList *fields) {
  QStringList out;
  QString field;
  bool quoted = false, wasQuoted = false;
  for (qsizetype i = 0; i < line.size(); ++i) {
    const QChar c = line.at(i);
    if (quoted) {
      if (c == QLatin1Char('"')) {
        if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
          field += c;
          ++i;
        } else {
          quoted = false;
        }
      } else {
        field += c;
      }
    } else if (c == QLatin1Char('"')) {
      if (!field.isEmpty() || wasQuoted)
        return false;
      quoted = wasQuoted = true;
    } else if (c == QLatin1Char(',')) {
      out << field;
      field.clear();
      wasQuoted = false;
    } else {
      if (wasQuoted)
        return false;
      field += c;
    }
  }
  if (quoted)
    return false;
  out << field;
  if (fields)
    *fields = out;
  return true;
}
bool csvParse(const QString &text, QList<Entry> *entries, QString *error,
              const QDate &today) {
  QString input = text;
  if (input.startsWith(QChar(0xfeff)))
    input.remove(0, 1);
  const QStringList lines = input.split(QLatin1Char('\n'));
  QStringList header;
  if (lines.isEmpty() || !csvSplit(lines.first().trimmed(), &header) ||
      header != QStringList{QStringLiteral("date"), QStringLiteral("weight_kg")}) {
    if (error)
      *error = QStringLiteral("Expected header: date,weight_kg");
    return false;
  }
  QList<Entry> parsed;
  QSet<QDate> seen;
  for (int i = 1; i < lines.size(); ++i) {
    QString line = lines[i];
    if (line.endsWith(QLatin1Char('\r')))
      line.chop(1);
    if (line.isEmpty())
      continue;
    QStringList fields;
    QDate date;
    int grams;
    if (!csvSplit(line, &fields) || fields.size() != 2 ||
        !parseDate(fields.value(0).trimmed(), &date, nullptr, today) ||
        !parseWeight(fields.value(1), &grams) || seen.contains(date)) {
      if (error)
        *error = QStringLiteral("Invalid or duplicate record on line %1.")
                     .arg(i + 1);
      return false;
    }
    seen.insert(date);
    parsed << Entry{date, grams, {}, {}};
  }
  std::sort(parsed.begin(), parsed.end(),
            [](const Entry &a, const Entry &b) { return a.date < b.date; });
  if (entries)
    *entries = parsed;
  return true;
}
} // namespace pw
