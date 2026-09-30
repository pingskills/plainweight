#pragma once
#include "database/store.h"
#include <QAbstractListModel>
#include <QObject>
#include <QVariantList>

namespace pw {
class HistoryModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles { DateRole = Qt::UserRole + 1, WeightRole, ChangeRole };
  int rowCount(const QModelIndex &parent = {}) const override {
    return parent.isValid() ? 0 : m_entries.size();
  }
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override {
    return {{DateRole, "entryDate"},
            {WeightRole, "weight"},
            {ChangeRole, "change"}};
  }
  void setEntries(const QList<Entry> &ascending);
  QDate dateAt(int row) const;

private:
  QList<Entry> m_entries;
};

class Controller : public QObject {
  Q_OBJECT
  Q_PROPERTY(HistoryModel *history READ history CONSTANT)
  Q_PROPERTY(QVariantList graph READ graph NOTIFY changed)
  Q_PROPERTY(QString latest READ latest NOTIFY changed)
  Q_PROPERTY(QString latestDate READ latestDate NOTIFY changed)
  Q_PROPERTY(QString previousChange READ previousChange NOTIFY changed)
  Q_PROPERTY(QString weekChange READ weekChange NOTIFY changed)
  Q_PROPERTY(QString monthChange READ monthChange NOTIFY changed)
  Q_PROPERTY(QString overallChange READ overallChange NOTIFY changed)
  Q_PROPERTY(QString lowest READ lowest NOTIFY changed)
  Q_PROPERTY(QString highest READ highest NOTIFY changed)
  Q_PROPERTY(QString target READ target NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)
public:
  explicit Controller(const QString &path, QObject *parent = nullptr);
  bool initialize();
  HistoryModel *history() { return &m_history; }
  QVariantList graph() const;
  QString latest() const;
  QString latestDate() const;
  QString previousChange() const;
  QString weekChange() const;
  QString monthChange() const;
  QString overallChange() const;
  QString lowest() const;
  QString highest() const;
  QString target() const;
  QString error() const { return m_error; }
  Q_INVOKABLE QString today() const {
    return QDate::currentDate().toString(Qt::ISODate);
  }
  Q_INVOKABLE bool exists(const QString &date) const;
  Q_INVOKABLE QString weightOn(const QString &date) const;
  Q_INVOKABLE bool save(const QString &date, const QString &weight,
                        bool update);
  Q_INVOKABLE bool edit(const QString &oldDate, const QString &date,
                        const QString &weight);
  Q_INVOKABLE bool remove(const QString &date);
  Q_INVOKABLE bool setTarget(const QString &value);
  Q_INVOKABLE bool backup(const QString &path);
  Q_INVOKABLE QString inspectBackup(const QString &path);
  Q_INVOKABLE bool restore(const QString &path);
  Q_INVOKABLE bool exportCsv(const QString &path);
  Q_INVOKABLE bool importCsv(const QString &path);
  Q_INVOKABLE QString historyDate(int index) const {
    return m_history.dateAt(index).toString(Qt::ISODate);
  }
  Q_INVOKABLE QString dataPath() const { return m_store.path(); }
signals:
  void changed();

private:
  void refresh();
  bool fail(const QString &reason);
  Store m_store;
  QString m_path;
  QString m_error;
  QList<Entry> m_entries;
  HistoryModel m_history;
};
} // namespace pw
