#pragma once

namespace pw {
class Controller;
class Theme;
// Fusion, unless the user chose a style via QT_QUICK_CONTROLS_STYLE. Call
// before creating the QML engine. Shared by the app and the UI tests so the
// tests exercise the style that ships.
void configureStyle();
// Exposes the controller and theme to QML as "PlainWeight.Core" singletons.
void registerSingletons(Controller *controller, Theme *theme);
} // namespace pw
