#include "config.h"
#include "database/store.h"
#include "services/cli.h"
#include "services/controller.h"
#include "ui/theme.h"
#include "ui/uisetup.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QTextStream>

Q_IMPORT_QML_PLUGIN(PlainWeightPlugin)
static void identity() {
  // applicationName determines the data directory: ~/.local/share/plainweight
  QCoreApplication::setApplicationName(QStringLiteral("plainweight"));
  QCoreApplication::setApplicationVersion(QStringLiteral(PLAINWEIGHT_VERSION));
}
int main(int argc, char **argv) {
  // Only PlainWeight's own commands run headless; anything else (including
  // standard Qt options such as -platform or -style) starts the GUI.
  if (pw::cli::isCliInvocation(argc, argv)) {
    QCoreApplication app(argc, argv);
    identity();
    QTextStream out(stdout), err(stderr);
    return pw::cli::run(app.arguments(), pw::databasePath(), out, err);
  }
  QGuiApplication app(argc, argv);
  identity();
  QGuiApplication::setApplicationDisplayName(QStringLiteral("PlainWeight"));
  QGuiApplication::setDesktopFileName(QStringLiteral(PLAINWEIGHT_APP_ID));
  QGuiApplication::setWindowIcon(
      QIcon(QStringLiteral(":/qt/qml/PlainWeight/resources/icons/png/256/"
                           "io.github.pingskills.plainweight.png")));
  pw::configureStyle();
  pw::Controller controller(pw::databasePath());
  controller.initialize();
  pw::Theme theme;
  pw::registerSingletons(&controller, &theme);
  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.loadFromModule(QStringLiteral("PlainWeight"), QStringLiteral("Main"));
  return app.exec();
}
