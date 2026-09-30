#include "services/controller.h"
#include <QFileInfo>
#include <algorithm>

namespace pw {
static QString delta(int grams) {
  return (grams > 0 ? QStringLiteral("+") : QString()) +
         QString::number(grams / 1000.0, 'f', 1) + QStringLiteral(" kg");
}
QVariant HistoryModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
    return {};
  int i = index.row();
  const Entry &e = m_entries[i];
  if (role == DateRole)
    return e.date.toString(QStringLiteral("dd MMM yyyy"));
  if (role == WeightRole)
    return displayWeight(e.grams) + QStringLiteral(" kg");
  if (role == ChangeRole)
    return i + 1 < m_entries.size() ? delta(e.grams - m_entries[i + 1].grams)
                                    : QStringLiteral("—");
  return {};
}
void HistoryModel::setEntries(const QList<Entry> &ascending) {
  beginResetModel();
  m_entries = ascending;
  std::reverse(m_entries.begin(), m_entries.end());
  endResetModel();
}
QDate HistoryModel::dateAt(int row) const {
  return row >= 0 && row < m_entries.size() ? m_entries[row].date : QDate();
}
Controller::Controller(const QString &path, QObject *parent)
    : QObject(parent), m_path(path) {}
bool Controller::fail(const QString &reason) {
  m_error = reason;
  emit changed();
  return false;
}
bool Controller::initialize() {
  if (!m_store.open(m_path))
    return fail(m_store.error());
  refresh();
  return true;
}
void Controller::refresh() {
  m_entries = m_store.entries();
  m_history.setEntries(m_entries);
  m_error.clear();
  emit changed();
}
QString Controller::latest() const {
  return m_entries.isEmpty()
             ? QStringLiteral("—")
             : displayWeight(m_entries.last().grams) + QStringLiteral(" kg");
}
QString Controller::latestDate() const {
  return m_entries.isEmpty()
             ? QStringLiteral("No measurements yet")
             : m_entries.last().date.toString(QStringLiteral("dd MMM yyyy"));
}
QString Controller::previousChange() const {
  return m_entries.size() < 2 ? QStringLiteral("Not enough measurements")
                              : delta(m_entries.last().grams -
                                      m_entries[m_entries.size() - 2].grams);
}
QString Controller::weekChange() const {
  auto d = changeAt(m_entries, 7);
  return d ? delta(*d) : QStringLiteral("No measurement near 7 days earlier");
}
QString Controller::monthChange() const {
  auto d = changeAt(m_entries, 30, 7);
  return d ? delta(*d) : QStringLiteral("No measurement near 30 days earlier");
}
QString Controller::overallChange() const {
  return m_entries.size() < 2
             ? QStringLiteral("—")
             : delta(m_entries.last().grams - m_entries.first().grams);
}
QString Controller::lowest() const {
  if (m_entries.isEmpty())
    return QStringLiteral("—");
  auto it = std::min_element(
      m_entries.begin(), m_entries.end(),
      [](const Entry &a, const Entry &b) { return a.grams < b.grams; });
  return displayWeight(it->grams) + QStringLiteral(" kg");
}
QString Controller::highest() const {
  if (m_entries.isEmpty())
    return QStringLiteral("—");
  auto it = std::max_element(
      m_entries.begin(), m_entries.end(),
      [](const Entry &a, const Entry &b) { return a.grams < b.grams; });
  return displayWeight(it->grams) + QStringLiteral(" kg");
}
QString Controller::target() const {
  auto v = m_store.target();
  return v ? displayWeight(*v) : QString();
}
QVariantList Controller::graph() const {
  QVariantList list;
  for (const auto &p : points(m_entries)) {
    QVariantMap item{{QStringLiteral("date"), p.date.toString(Qt::ISODate)},
                     {QStringLiteral("kg"), p.kg}};
    if (p.trend)
      item.insert(QStringLiteral("trend"), *p.trend);
    list << item;
  }
  return list;
}
bool Controller::exists(const QString &date) const {
  for (const auto &e : m_entries)
    if (e.date.toString(Qt::ISODate) == date)
      return true;
  return false;
}
QString Controller::weightOn(const QString &date) const {
  for (const auto &e : m_entries)
    if (e.date.toString(Qt::ISODate) == date)
      return QString::number(e.grams / 1000.0, 'f', 3);
  return {};
}
bool Controller::save(const QString &date, const QString &weight, bool update) {
  QDate d;
  int g;
  QString error;
  if (!parseDate(date, &d, &error) || !parseWeight(weight, &g, &error))
    return fail(error);
  if (!m_store.save(d, g, update))
    return fail(m_store.error());
  refresh();
  return true;
}
bool Controller::edit(const QString &oldDate, const QString &date,
                      const QString &weight) {
  QDate old, d;
  int g;
  QString error;
  if (!parseDate(oldDate, &old, &error) || !parseDate(date, &d, &error) ||
      !parseWeight(weight, &g, &error))
    return fail(error);
  if (!m_store.edit(old, d, g))
    return fail(m_store.error());
  refresh();
  return true;
}
bool Controller::remove(const QString &date) {
  QDate d;
  QString error;
  if (!parseDate(date, &d, &error))
    return fail(error);
  if (!m_store.remove(d))
    return fail(m_store.error());
  refresh();
  return true;
}
bool Controller::setTarget(const QString &value) {
  int g;
  QString error;
  std::optional<int> v;
  if (!value.trimmed().isEmpty()) {
    if (!parseWeight(value, &g, &error))
      return fail(error);
    v = g;
  }
  if (!m_store.setTarget(v))
    return fail(m_store.error());
  refresh();
  return true;
}
bool Controller::backup(const QString &path) {
  if (!m_store.backup(path))
    return fail(m_store.error());
  return true;
}
QString Controller::inspectBackup(const QString &path) {
  int count;
  QString error;
  if (!Store::inspect(path, &count, &error)) {
    fail(error);
    return {};
  }
  return QStringLiteral("%1 measurements").arg(count);
}
bool Controller::restore(const QString &path) {
  QString safety;
  if (!m_store.restore(path, &safety))
    return fail(m_store.error());
  refresh();
  return true;
}
bool Controller::exportCsv(const QString &path) {
  if (!m_store.exportCsv(path))
    return fail(m_store.error());
  return true;
}
bool Controller::importCsv(const QString &path) {
  if (!m_store.importCsv(path))
    return fail(m_store.error());
  refresh();
  return true;
}
} // namespace pw
