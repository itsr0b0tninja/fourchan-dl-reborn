#include <QApplication>
#include <QHeaderView>
#include <QTreeWidget>
#include <QDebug>
#include "overviewstatus.h"

static bool check(const QString &actual, const char *expected, const char *test) {
    if (actual == QLatin1String(expected)) return true;
    qCritical() << test << "expected" << expected << "got" << actual;
    return false;
}

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setStyle("fusion");

    // All categories must be present, including those with zero threads.
    const QStringList keys = OverviewStatus::keys();
    if (keys.size() != 17 || keys.first() != "all" ||
            !keys.contains("ended") || !keys.contains("retry") ||
            !keys.contains("errors") || !keys.contains("other"))
        return 1;
    const struct { const char *status; const char *category; } cases[] = {
        {"", "idle"}, {"idle", "idle"}, {"Running", "running"},
        {"downloading", "downloading"}, {"Finished", "finished"},
        {"Finished (retry 23:45:01)", "retry"},
        {"Incomplete: 2 failed, 3 missing. Retry Failed to try again.", "incomplete"},
        {"Cached", "cached"}, {"Rendering", "rendering"},
        {"rescanning", "rescanning"}, {"Parsing", "parsing"},
        {"Stopped", "stopped"}, {"Blocked", "blocked"},
        {"Thread removed; pending attachments may still finish", "ended"},
        {"Thread removed", "ended"}, {"Banned", "errors"},
        {"Moving files...", "moving"}, {"Move failed", "errors"},
        {"Thread request failed; retry with rescan", "errors"},
        {"No parser available", "errors"},
        {"Directory does not exist / Could not be created", "errors"},
        {"Thread already exists", "errors"},
        {"New unknown future status", "other"}
    };
    for (const auto &test : cases) {
        if (!check(OverviewStatus::categoryFor(QLatin1String(test.status)),
                   test.category, test.status))
            return 2;
    }

    // Actual Qt headers must remain manually sized even for long statuses
    // or after changing the Status header to show an active filter.
    QTreeWidget overview;
    overview.setColumnCount(4);
    overview.setHeaderLabels(QStringList() << "Name" << "Images" << "Status" << "URI");
    overview.header()->setStretchLastSection(false);
    for (int c = 0; c < 4; ++c)
        overview.header()->setSectionResizeMode(c, QHeaderView::Interactive);
    overview.setColumnWidth(0, 140);
    overview.setColumnWidth(1, 55);
    overview.setColumnWidth(2, 48);
    overview.setColumnWidth(3, 90);
    QTreeWidgetItem *first = new QTreeWidgetItem(&overview);
    first->setText(2, "Finished (retry 23:45:01)");
    QTreeWidgetItem *second = new QTreeWidgetItem(&overview);
    second->setText(2, "Thread removed");
    overview.headerItem()->setText(2, "Status (Ended / Removed)");
    overview.show();
    QApplication::processEvents();
    if (overview.columnWidth(2) != 48 || overview.columnWidth(3) != 90 ||
            overview.header()->sectionResizeMode(2) != QHeaderView::Interactive)
        return 3;

    // Filtering rows must not reorder or destroy them. The association with
    // the corresponding open tab remains stable, even for an empty filter.
    first->setHidden(true);
    second->setHidden(false);
    if (overview.indexOfTopLevelItem(second) != 1 ||
            overview.topLevelItemCount() != 2 || !first->isHidden() ||
            second->isHidden())
        return 4;
    first->setHidden(true);
    second->setHidden(true);
    if (overview.topLevelItemCount() != 2 || !first->isHidden() || !second->isHidden())
        return 5;
    first->setHidden(false);
    second->setHidden(false);
    if (overview.indexOfTopLevelItem(first) != 0 ||
            overview.indexOfTopLevelItem(second) != 1)
        return 6;

    qInfo() << "PASS: complete status taxonomy, variable statuses,"
               " zero-result filtering, stable row identity and manual widths";
    return 0;
}
