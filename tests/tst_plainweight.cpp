#include "core/weight.h"
#include "database/store.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class Tests : public QObject {
  Q_OBJECT
private slots:
  void validation() {
    int g = 0;
    QDate d;
    QVERIFY(pw::parseWeight("82.456", &g));
    QCOMPARE(g, 82456);
    QVERIFY(!pw::parseWeight("0", &g));
    QVERIFY(!pw::parseWeight("82.4567", &g));
    QVERIFY(!pw::parseWeight("nan", &g));
    QVERIFY(pw::parseDate("2026-09-30", &d));
    QVERIFY(!pw::parseDate("2026-02-30", &d));
  }
  void calculations() {
    QDate now = QDate::currentDate();
    QList<pw::Entry> list = {{now.addDays(-30), 83000},
                             {now.addDays(-7), 82500},
                             {now.addDays(-6), 82400},
                             {now.addDays(-2), 82000},
                             {now, 81800}};
    QCOMPARE(pw::changeAt(list, 7), std::optional<int>(-700));
    QCOMPARE(pw::changeAt(list, 30), std::optional<int>(-1200));
    auto p = pw::points(list);
    QVERIFY(!p[0].trend);
    QVERIFY(!p[1].trend);
    QVERIFY(!p[2].trend);
    QVERIFY(p[4].trend);
    QCOMPARE(*p[4].trend, (82400 + 82000 + 81800) / 3000.0);
    QList<pw::Entry> sparse = {{now.addDays(-100), 90000}, {now, 80000}};
    QVERIFY(!pw::changeAt(sparse, 7));
    QVERIFY(!pw::points(sparse)[1].trend);
  }
  void databaseAndFiles() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.filePath("test.db");
    pw::Store store;
    QVERIFY2(store.open(path), qPrintable(store.error()));
    QVERIFY(store.entries().isEmpty());
    QDate d = QDate::currentDate();
    QVERIFY(store.save(d, 82400));
    QVERIFY(!store.save(d, 82500));
    QVERIFY(store.save(d, 82500, true));
    QCOMPARE(store.entries().first().grams, 82500);
    QDate earlier = d.addDays(-7);
    QVERIFY(store.edit(d, earlier, 82456));
    QCOMPARE(store.entries().first().date, earlier);
    QCOMPARE(store.entries().first().grams, 82456);
    QVERIFY(store.edit(earlier, d, 82500));
    QVERIFY(store.setTarget(75000));
    QCOMPARE(store.target(), std::optional<int>(75000));
    QString csv = dir.filePath("data.csv");
    QVERIFY(store.exportCsv(csv));
    QFile file(csv);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(QString::fromUtf8(file.readAll()).contains("date,weight_kg"));
    file.close();
    QString backup = dir.filePath("backup.db");
    QVERIFY2(store.backup(backup), qPrintable(store.error()));
    int count = 0;
    QString reason;
    QVERIFY(pw::Store::inspect(backup, &count, &reason));
    QCOMPARE(count, 1);
    QVERIFY(store.remove(d));
    QVERIFY(store.entries().isEmpty());
    QVERIFY(store.importCsv(csv));
    QCOMPARE(store.entries().first().grams, 82500);
    QVERIFY(store.remove(d));
    QString safety;
    QVERIFY2(store.restore(backup, &safety), qPrintable(store.error()));
    QCOMPARE(store.entries().first().grams, 82500);
    QVERIFY(QFile::exists(safety));
    store.close();
    QVERIFY(store.open(path));
    QCOMPARE(store.entries().first().grams, 82500);
    QVERIFY(!pw::Store::inspect(csv, &count, &reason));
  }
  void csvConflicts() {
    QTemporaryDir dir;
    pw::Store store;
    QVERIFY(store.open(dir.filePath("test.db")));
    QDate d = QDate::currentDate();
    QVERIFY(store.save(d, 80000));
    QString file = dir.filePath("bad.csv");
    {
      QFile f(file);
      QVERIFY(f.open(QIODevice::WriteOnly));
      f.write(("date,weight_kg\n" + d.toString(Qt::ISODate) + ",81.000\n")
                  .toUtf8());
    }
    QVERIFY(!store.importCsv(file));
    QCOMPARE(store.entries().first().grams, 80000);
    QList<pw::Entry> parsed;
    QString error;
    QVERIFY(
        !pw::csvParse("date,weight_kg\n2026-02-30,80.0\n", &parsed, &error));
    QVERIFY(!pw::csvParse("date,weight_kg\n2026-09-01,80.0\n2026-09-01,81.0\n",
                          &parsed, &error));
  }
  void cliAndSchema() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString appDir = dir.filePath(QStringLiteral("plainweight"));
    QVERIFY(QDir().mkpath(appDir));
    QString path = QDir(appDir).filePath(QStringLiteral("plainweight.db"));
    pw::Store store;
    QVERIFY(store.open(path));
    QDate today = QDate::currentDate();
    QVERIFY(store.save(today.addDays(-7), 83000));
    QVERIFY(store.save(today, 82456));
    store.close();
    QProcess process;
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("XDG_DATA_HOME"), dir.path());
    process.setProcessEnvironment(env);
    process.start(QCoreApplication::applicationDirPath() +
                      QStringLiteral("/plainweight"),
                  {QStringLiteral("--current")});
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(QString::fromUtf8(process.readAllStandardOutput()),
             today.toString(Qt::ISODate) + QStringLiteral("\t82.456\n"));
    process.start(QCoreApplication::applicationDirPath() +
                      QStringLiteral("/plainweight"),
                  {QStringLiteral("--change-week")});
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(QString::fromUtf8(process.readAllStandardOutput()),
             QStringLiteral("-0.544\n"));
    {
      QSqlDatabase db = QSqlDatabase::addDatabase(
          QStringLiteral("QSQLITE"), QStringLiteral("schema-test"));
      db.setDatabaseName(path);
      QVERIFY(db.open());
      QSqlQuery q(db);
      QVERIFY(q.exec(QStringLiteral("PRAGMA application_id")));
      QVERIFY(q.next());
      QCOMPARE(q.value(0).toInt(), pw::Store::ApplicationId);
      q.finish();
      QVERIFY(q.exec(QStringLiteral("PRAGMA user_version")));
      QVERIFY(q.next());
      QCOMPARE(q.value(0).toInt(), pw::Store::SchemaVersion);
      q.finish();
      QVERIFY(q.exec(QStringLiteral("PRAGMA user_version = 2")));
      q.finish();
      db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("schema-test"));
    QVERIFY(!store.open(path));
    QVERIFY(store.error().contains(QStringLiteral("newer")));
    QVERIFY(QFile::exists(path));
  }
};
QTEST_GUILESS_MAIN(Tests)
#include "tst_plainweight.moc"
