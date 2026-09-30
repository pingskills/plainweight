#include "omarchy/omarchycolors.h"
#include "ui/theme.h"
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QtTest>

// Omarchy discovery/parsing, the declared light/dark mode, and the contrast
// guarantee for secondary text.
class TestTheme : public QObject {
  Q_OBJECT
  static void writeColors(const QString &dir, const QByteArray &toml) {
    const QString themeDir = dir + "/state/omarchy/current/theme";
    QVERIFY(QDir().mkpath(themeDir));
    QFile f(themeDir + "/colors.toml");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(toml);
  }
private slots:
  void parsesColorsToml() {
    const auto v = pw::omarchy::parseColorsToml(
        "mode = \"dark\"\n# comment\naccent = \"#7aa2f7\"  # trailing\n"
        "background='#1a1b26'\n[table]\nbroken line\nForeground = \"#a9b1d6\"\n");
    QCOMPARE(v.value("mode"), QStringLiteral("dark"));
    QCOMPARE(v.value("accent"), QStringLiteral("#7aa2f7"));
    QCOMPARE(v.value("background"), QStringLiteral("#1a1b26"));
    QCOMPARE(v.value("foreground"), QStringLiteral("#a9b1d6"));
    QCOMPARE(v.size(), 4);
    QVERIFY(pw::omarchy::parseColorsToml("= = =\n[[[").isEmpty());
  }
  void locatesThemeAndCanBeDisabled() {
    QTemporaryDir dir;
    qputenv("XDG_STATE_HOME", (dir.path() + "/state").toUtf8());
    qputenv("XDG_CONFIG_HOME", (dir.path() + "/config").toUtf8());
    QVERIFY(pw::omarchy::activeColorsFile().isEmpty()); // absent: no crash
    writeColors(dir.path(), "background = \"#ffffff\"\n");
    QVERIFY(pw::omarchy::activeColorsFile().endsWith("colors.toml"));
    qputenv("PLAINWEIGHT_NO_OMARCHY", "1");
    QVERIFY(pw::omarchy::disabledByEnvironment());
    qunsetenv("PLAINWEIGHT_NO_OMARCHY");
    QVERIFY(!pw::omarchy::disabledByEnvironment());
  }
  void followsDeclaredMode() {
    QTemporaryDir dir;
    qputenv("XDG_STATE_HOME", (dir.path() + "/state").toUtf8());
    qunsetenv("PLAINWEIGHT_NO_OMARCHY");
    // A mid-grey background is ambiguous; the declared mode decides.
    writeColors(dir.path(), "mode = \"light\"\nbackground = \"#7a7a7a\"\n"
                            "foreground = \"#101010\"\naccent = \"#2255aa\"\n");
    pw::Theme light;
    QVERIFY(!light.dark());
    QCOMPARE(light.background(), QColor("#7a7a7a"));
    QCOMPARE(light.accent(), QColor("#2255aa"));
    writeColors(dir.path(), "mode = \"dark\"\nbackground = \"#7a7a7a\"\n"
                            "foreground = \"#f0f0f0\"\n");
    pw::Theme dark;
    QVERIFY(dark.dark());
  }
  void mutedTextMeetsWcagAA_data() {
    QTest::addColumn<QColor>("text");
    QTest::addColumn<QColor>("background");
    QTest::newRow("tokyo night") << QColor("#a9b1d6") << QColor("#1a1b26");
    QTest::newRow("light") << QColor("#1f2328") << QColor("#fbfbfa");
    QTest::newRow("low-contrast theme") << QColor("#9aa0a6") << QColor("#3c4043");
    QTest::newRow("gruvbox") << QColor("#d4be98") << QColor("#282828");
  }
  void mutedTextMeetsWcagAA() {
    QFETCH(QColor, text);
    QFETCH(QColor, background);
    const QColor muted = pw::Theme::mutedText(text, background);
    // Secondary text keeps AA contrast whenever the theme's own text does.
    if (pw::Theme::contrast(text, background) >= 4.5)
      QVERIFY(pw::Theme::contrast(muted, background) >= 4.5);
    else
      QCOMPARE(muted, text);
  }
};
QTEST_MAIN(TestTheme)
#include "tst_theme.moc"
