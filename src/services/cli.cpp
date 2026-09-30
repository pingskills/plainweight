#include "services/cli.h"
#include "config.h"
#include "core/weight.h"
#include "database/store.h"
#include <cstring>

namespace pw::cli {
namespace {
const char *const Commands[] = {"--version", "-v",          "--help",
                                "-h",        "--current",   "--change-week",
                                "--change-month"};
const char *const Help =
    "Usage: plainweight [--current | --change-week | --change-month | "
    "--version | --help]\n"
    "\n"
    "  --current        latest measurement: YYYY-MM-DD<TAB>kg\n"
    "  --change-week    change in kg versus about 7 days earlier\n"
    "  --change-month   change in kg versus about 30 days earlier\n"
    "\n"
    "With no options, PlainWeight opens its window.\n"
    "Exit codes: 0 success, 1 data error, 2 usage error, 3 no suitable "
    "measurement.\n";
} // namespace

bool isCliInvocation(int argc, char **argv) {
  for (int i = 1; i < argc; ++i)
    for (const char *cmd : Commands)
      if (std::strcmp(argv[i], cmd) == 0)
        return true;
  return false;
}

int run(const QStringList &arguments, const QString &databasePath,
        QTextStream &out, QTextStream &err) {
  if (arguments.size() != 2) {
    err << "One option at a time. See --help.\n";
    return ExitUsage;
  }
  const QString arg = arguments.at(1);
  if (arg == QLatin1String("--version") || arg == QLatin1String("-v")) {
    out << "plainweight " << PLAINWEIGHT_VERSION << "\n";
    return ExitOk;
  }
  if (arg == QLatin1String("--help") || arg == QLatin1String("-h")) {
    out << Help;
    return ExitOk;
  }
  if (arg != QLatin1String("--current") &&
      arg != QLatin1String("--change-week") &&
      arg != QLatin1String("--change-month")) {
    err << "Unknown option: " << arg << ". See --help.\n";
    return ExitUsage;
  }

  // Read-only: never creates the database.
  Store store;
  if (!store.open(databasePath, true)) {
    if (store.openError() == Store::OpenError::Missing)
      return ExitNothing;
    err << "plainweight: " << store.error() << "\n";
    return ExitDataError;
  }
  const QList<Entry> entries = store.entries();
  if (entries.isEmpty())
    return ExitNothing;
  if (arg == QLatin1String("--current")) {
    const Entry &e = entries.last();
    out << e.date.toString(Qt::ISODate) << '\t'
        << QString::number(e.grams / 1000.0, 'f', 3) << "\n";
    return ExitOk;
  }
  const auto change = arg == QLatin1String("--change-week")
                          ? changeAt(entries, 7)
                          : changeAt(entries, 30, 7);
  if (!change)
    return ExitNothing;
  out << QString::number(*change / 1000.0, 'f', 3) << "\n";
  return ExitOk;
}
} // namespace pw::cli
