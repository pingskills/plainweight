#include "services/controller.h"
#include "ui/theme.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>

Q_IMPORT_QML_PLUGIN(PlainWeightPlugin)
class UiSmoke : public QObject {
  Q_OBJECT
private slots:
  void entryWorkflow() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    pw::Controller controller(dir.filePath(QStringLiteral("test.db")));
    QVERIFY(controller.initialize());
    pw::Theme theme;
    qmlRegisterSingletonInstance("PlainWeight.Core", 1, 0, "App", &controller);
    qmlRegisterSingletonInstance("PlainWeight.Core", 1, 0, "Theme", &theme);
    QQmlApplicationEngine engine;
    engine.loadFromModule("PlainWeight", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);
    QObject *window = engine.rootObjects().first();
    QObject *date = window->findChild<QObject *>("dateField");
    QObject *weight = window->findChild<QObject *>("weightField");
    QVERIFY(date);
    QVERIFY(weight);
    date->setProperty("text", QDate::currentDate().toString(Qt::ISODate));
    weight->setProperty("text", QStringLiteral("82.4"));
    QVERIFY(QMetaObject::invokeMethod(window, "saveEntry"));
    QCOMPARE(controller.history()->rowCount(), 1);
    QCOMPARE(controller.latest(), QStringLiteral("82.4 kg"));
    const QString screenshot = qEnvironmentVariable("PLAINWEIGHT_SCREENSHOT");
    if (!screenshot.isEmpty()) {
      QTest::qWait(250);
      auto *quick = qobject_cast<QQuickWindow *>(window);
      QVERIFY(quick);
      QVERIFY(quick->grabWindow().save(screenshot));
    }
  }
};
QTEST_MAIN(UiSmoke)
#include "tst_ui_smoke.moc"
