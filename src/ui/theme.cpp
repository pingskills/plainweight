#include "ui/theme.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QRegularExpression>
#include <QStyleHints>

namespace pw {
Theme::Theme(QObject *parent) : QObject(parent) {
  m_timer.setSingleShot(true);
  m_timer.setInterval(250);
  connect(&m_timer, &QTimer::timeout, this, &Theme::reload);
  connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_timer,
          qOverload<>(&QTimer::start));
  connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_timer,
          qOverload<>(&QTimer::start));
  connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
          [this] { reload(); });
  reload();
}
qreal Theme::fontSize() const {
  auto f = QGuiApplication::font();
  return f.pointSizeF() > 0 ? f.pointSizeF() : 10.0;
}
void Theme::reload() {
  auto palette = QGuiApplication::palette();
  m_background = palette.color(QPalette::Window);
  m_text = palette.color(QPalette::WindowText);
  m_input = palette.color(QPalette::Base);
  m_accent = palette.color(QPalette::Highlight);
  if (qEnvironmentVariableIsEmpty("PLAINWEIGHT_NO_OMARCHY")) {
    const QString rel = QStringLiteral("omarchy/current/theme/colors.toml");
    const QString state = qEnvironmentVariable(
        "XDG_STATE_HOME", QDir::homePath() + QStringLiteral("/.local/state"));
    const QString config = qEnvironmentVariable(
        "XDG_CONFIG_HOME", QDir::homePath() + QStringLiteral("/.config"));
    QStringList candidates = {QDir(state).filePath(rel),
                              QDir(config).filePath(rel)};
    for (const auto &path : candidates) {
      QFile file(path);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        continue;
      QHash<QString, QString> values;
      QRegularExpression re(
          QStringLiteral(R"(^\s*([\w-]+)\s*=\s*[\"']([^\"']+)[\"'])"));
      for (const auto &line :
           QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))) {
        auto match = re.match(line);
        if (match.hasMatch())
          values.insert(match.captured(1), match.captured(2));
      }
      QColor bg(values.value(QStringLiteral("background"))),
          fg(values.value(QStringLiteral("foreground"))),
          accent(values.value(QStringLiteral("accent")));
      if (bg.isValid() && fg.isValid()) {
        m_background = bg;
        m_text = fg;
        if (accent.isValid())
          m_accent = accent;
        m_input = bg.lighter(bg.lightnessF() < 0.5 ? 135 : 105);
      }
      break;
    }
    if (!m_watcher.files().isEmpty())
      m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty())
      m_watcher.removePaths(m_watcher.directories());
    QStringList watch;
    for (const auto &path : candidates) {
      for (const auto &p :
           {path, QFileInfo(path).absolutePath(),
            QFileInfo(QFileInfo(path).absolutePath()).absolutePath(),
            QFileInfo(QFileInfo(QFileInfo(path).absolutePath()).absolutePath())
                .absolutePath()})
        if (QFileInfo::exists(p))
          watch << p;
    }
    if (!watch.isEmpty())
      m_watcher.addPaths(watch);
  }
  const bool dark = m_background.lightnessF() < 0.5;
  m_surface = dark ? m_background.lighter(112) : m_background.darker(103);
  m_border = dark ? m_background.lighter(165) : m_background.darker(125);
  m_muted = dark ? m_text.darker(125) : m_text.lighter(160);
  m_onAccent =
      m_accent.lightnessF() < 0.55 ? QColor(Qt::white) : QColor(Qt::black);
  emit changed();
}
} // namespace pw
