#include "ui/uisetup.h"
#include "services/controller.h"
#include "ui/theme.h"
#include <QQmlEngine>
#include <QQuickStyle>

namespace pw {
void configureStyle() {
  if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
    QQuickStyle::setStyle(QStringLiteral("Fusion"));
}
void registerSingletons(Controller *controller, Theme *theme) {
  qmlRegisterSingletonInstance("PlainWeight.Core", 1, 0, "App", controller);
  qmlRegisterSingletonInstance("PlainWeight.Core", 1, 0, "Theme", theme);
}
} // namespace pw
