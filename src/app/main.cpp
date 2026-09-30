#include "config.h"
#include "core/weight.h"
#include "database/store.h"
#include "services/controller.h"
#include "ui/theme.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickStyle>
#include <QTextStream>

Q_IMPORT_QML_PLUGIN(PlainWeightPlugin)
static void identity() {
  QCoreApplication::setApplicationName(QStringLiteral("plainweight"));
  QCoreApplication::setApplicationVersion(QStringLiteral(PLAINWEIGHT_VERSION));
}
int main(int argc, char **argv) {
  bool cli = argc > 1;
  if (cli) {
    QCoreApplication app(argc, argv);
    identity();
    QTextStream out(stdout), err(stderr);
    const QString arg = QString::fromLocal8Bit(argv[1]);
    if (argc != 2) {
      err << "One option at a time. See --help.\n";
      return 2;
    }
    if (arg == QLatin1String("--version")) {
      out << "plainweight " << PLAINWEIGHT_VERSION << "\n";
      return 0;
    }
    if (arg == QLatin1String("--help")) {
      out << "Usage: plainweight "
             "[--version|--current|--change-week|--change-month]\n";
      return 0;
    }
    if (arg != QLatin1String("--current") &&
        arg != QLatin1String("--change-week") &&
        arg != QLatin1String("--change-month")) {
      err << "Unknown option.\n";
      return 2;
    }
    pw::Store store;
    if (!store.open(pw::databasePath(), true)) {
      err << store.error() << "\n";
      return store.error() == QLatin1String("No measurements recorded.") ? 3
                                                                         : 1;
    }
    auto entries = store.entries();
    if (entries.isEmpty()) {
      err << "No measurements recorded.\n";
      return 3;
    }
    if (arg == QLatin1String("--current")) {
      const auto &e = entries.last();
      out << e.date.toString(Qt::ISODate) << '\t'
          << QString::number(e.grams / 1000.0, 'f', 3) << "\n";
      return 0;
    }
    auto value = arg == QLatin1String("--change-week")
                     ? pw::changeAt(entries, 7)
                     : pw::changeAt(entries, 30, 7);
    if (!value) {
      err << "No suitable historical measurement.\n";
      return 3;
    }
    out << QString::number(*value / 1000.0, 'f', 3) << "\n";
    return 0;
  }
  QGuiApplication app(argc, argv);
  identity();
  QGuiApplication::setApplicationDisplayName(QStringLiteral("PlainWeight"));
  QGuiApplication::setDesktopFileName(QStringLiteral(PLAINWEIGHT_APP_ID));
  QGuiApplication::setWindowIcon(
      QIcon(QStringLiteral(":/qt/qml/PlainWeight/resources/icons/png/256/"
                           "io.github.pingskills.plainweight.png")));
  if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
    QQuickStyle::setStyle(QStringLiteral("Fusion"));
  pw::Controller controller(pw::databasePath());
  controller.initialize();
  pw::Theme theme;
  qmlRegisterSingletonInstance("PlainWeight.Core", 1, 0, "App", &controller);
  qmlRegisterSingletonInstance("PlainWeight.Core", 1, 0, "Theme", &theme);
  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.loadFromModule(QStringLiteral("PlainWeight"), QStringLiteral("Main"));
  return app.exec();
}
