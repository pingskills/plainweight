#include "services/controller.h"
#include "ui/theme.h"
#include "ui/uisetup.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>

Q_IMPORT_QML_PLUGIN(PlainWeightPlugin)
// Loads the real interface in the shipped style (Fusion) and fails on any QML
// warning, then exercises the entry workflow.
class UiSmoke : public QObject {
  Q_OBJECT
private slots:
  void entryWorkflow() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    pw::configureStyle();
    pw::Controller controller(dir.filePath(QStringLiteral("test.db")));
    QVERIFY(controller.initialize());
    pw::Theme theme;
    pw::registerSingletons(&controller, &theme);
    QQmlApplicationEngine engine;
    QList<QQmlError> warnings;
    connect(&engine, &QQmlEngine::warnings, this,
            [&](const QList<QQmlError> &w) { warnings += w; });
    engine.loadFromModule("PlainWeight", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);
    QObject *window = engine.rootObjects().first();
    QObject *date = window->findChild<QObject *>("dateField");
    QObject *weight = window->findChild<QObject *>("weightField");
    QVERIFY(date);
    QVERIFY(weight);
    date->setProperty("text", controller.today());
    weight->setProperty("text", QStringLiteral("82,4")); // comma decimal
    QVERIFY(QMetaObject::invokeMethod(window, "saveEntry"));
    QCOMPARE(controller.history()->rowCount(), 1);
    QCOMPARE(controller.latest(), QStringLiteral("82.4 kg"));
    QVERIFY(controller.remove(controller.today()));
    QCOMPARE(controller.history()->rowCount(), 0);
    QTest::qWait(50);
    const QString screenshot = qEnvironmentVariable("PLAINWEIGHT_SCREENSHOT");
    if (!screenshot.isEmpty()) {
      QVERIFY(controller.save(controller.today(), QStringLiteral("82.4"), false));
      QTest::qWait(250);
      auto *quick = qobject_cast<QQuickWindow *>(window);
      QVERIFY(quick);
      QVERIFY(quick->grabWindow().save(screenshot));
    }
    QStringList messages;
    for (const QQmlError &e : warnings)
      messages << e.toString();
    QVERIFY2(warnings.isEmpty(), qPrintable(messages.join('\n')));
  }
};
QTEST_MAIN(UiSmoke)
#include "tst_ui_smoke.moc"
