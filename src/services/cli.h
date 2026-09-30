#pragma once
#include <QStringList>
#include <QTextStream>

namespace pw::cli {
inline constexpr int ExitOk = 0;
inline constexpr int ExitDataError = 1;
inline constexpr int ExitUsage = 2;
inline constexpr int ExitNothing = 3; // no suitable measurement

// True only when argv contains one of PlainWeight's own commands, so standard
// Qt GUI options (-platform, -style, session arguments) still start the GUI.
bool isCliInvocation(int argc, char **argv);

// Runs a read-only command. `arguments` includes the program name first.
int run(const QStringList &arguments, const QString &databasePath,
        QTextStream &out, QTextStream &err);
} // namespace pw::cli
