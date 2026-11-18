#ifndef OVERVIEWSTATUS_H
#define OVERVIEWSTATUS_H

#include <QString>
#include <QStringList>

// Stable filter IDs are saved to settings.ini. Keep this complete list even
// when a category has zero matching threads, so the menu stays predictable.
namespace OverviewStatus {

inline QStringList keys() {
    return QStringList()
        << "all" << "idle" << "running" << "downloading"
        << "retry" << "finished" << "incomplete" << "cached"
        << "rendering" << "rescanning" << "parsing" << "stopped"
        << "blocked" << "ended" << "moving" << "errors" << "other";
}

inline QString labelFor(const QString &key) {
    if (key == "all") return QStringLiteral("All statuses");
    if (key == "idle") return QStringLiteral("Idle");
    if (key == "running") return QStringLiteral("Running");
    if (key == "downloading") return QStringLiteral("Downloading");
    if (key == "retry") return QStringLiteral("Retry Pending");
    if (key == "finished") return QStringLiteral("Finished");
    if (key == "incomplete") return QStringLiteral("Incomplete");
    if (key == "cached") return QStringLiteral("Cached");
    if (key == "rendering") return QStringLiteral("Rendering");
    if (key == "rescanning") return QStringLiteral("Rescanning");
    if (key == "parsing") return QStringLiteral("Parsing");
    if (key == "stopped") return QStringLiteral("Stopped");
    if (key == "blocked") return QStringLiteral("Blocked");
    if (key == "ended") return QStringLiteral("Ended / Removed");
    if (key == "moving") return QStringLiteral("Moving");
    if (key == "errors") return QStringLiteral("Errors");
    return QStringLiteral("Other");
}

// Status strings can contain variable counts and retry clock times.
// Classify them by meaning rather than by their full displayed text.
// Every thread gets exactly one category; no active status is omitted.
inline QString categoryFor(const QString &raw) {
    const QString s = raw.trimmed().toLower();
    if (s.isEmpty() || s == "idle") return QStringLiteral("idle");
    if (s.startsWith("finished (retry") || s.startsWith("retry "))
        return QStringLiteral("retry");
    if (s.startsWith("thread removed") || s.startsWith("thread 404") ||
            s == "ended" || s.startsWith("thread ended"))
        return QStringLiteral("ended");
    if (s.startsWith("incomplete")) return QStringLiteral("incomplete");
    if (s == "downloading") return QStringLiteral("downloading");
    if (s == "finished") return QStringLiteral("finished");
    if (s == "cached") return QStringLiteral("cached");
    if (s == "rendering") return QStringLiteral("rendering");
    if (s == "rescanning") return QStringLiteral("rescanning");
    if (s == "parsing") return QStringLiteral("parsing");
    if (s == "stopped") return QStringLiteral("stopped");
    if (s == "blocked") return QStringLiteral("blocked");
    if (s.startsWith("moving files")) return QStringLiteral("moving");
    if (s == "running") return QStringLiteral("running");
    if (s == "banned" || s.contains("failed") || s.contains("error") ||
            s.contains("could not") || s.contains("no parser") ||
            s.contains("already exists"))
        return QStringLiteral("errors");
    return QStringLiteral("other");
}

} // namespace OverviewStatus

#endif // OVERVIEWSTATUS_H
