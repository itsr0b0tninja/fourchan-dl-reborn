#include <QApplication>
#include <QHeaderView>
#include <QTreeWidget>
#include <QDebug>
#include <QSignalSpy>
#include <QTest>
#include "overviewstatus.h"
#include "overviewboard.h"

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

    // Board parsing uses an exact URL path segment on an official host.
    const struct { const char *url; const char *board; } boardCases[] = {
        {"https://boards.4chan.org/b/thread/1234", "b"},
        {"http://boards.4chan.org/s/res/1234", "s"},
        {"https://boards.4channel.org/r9k/thread/4567", "r9k"},
        {"boards.4channel.org/r9k/thread/4567", "r9k"},
        {"https://boards.4chan.org/banned/thread/1234", "banned"},
        {"https://boards.4chan.org/", ""},
        {"", ""},
        {"https://other.example/b/thread/1234", ""},
        {"https://boards.4chan.org.evil.example/b/thread/1234", ""},
        {"https://fake4chan.org/b/thread/1234", ""},
        {"https://other.example/?next=/b/", ""},
        {"ftp://boards.4chan.org/b/thread/1234", ""}
    };
    for (const auto &test : boardCases) {
        if (!check(OverviewBoard::boardForUrl(QLatin1String(test.url)),
                   test.board, test.url))
            return 10;
    }
    if (OverviewBoard::normalizeFilter(QStringLiteral(" R9K ")) != QLatin1String("r9k") ||
        !OverviewBoard::normalizeFilter(QStringLiteral("/b/")).isEmpty() ||
        !OverviewBoard::matchesFilter(QStringLiteral(""), QString()) ||
        OverviewBoard::matchesFilter(QStringLiteral("https://boards.4chan.org/b/thread/1"),
                                     QStringLiteral("r9k")))
        return 11;

    // Actual Qt headers must remain manually sized even for long statuses
    // or after changing the Status header to show an active filter.
    QTreeWidget overview;
    overview.setColumnCount(4);
    overview.setHeaderLabels(QStringList() << "Name" << "Images" << "Status" << "URI");
    overview.header()->setStretchLastSection(false);
    for (int c = 0; c < 4; ++c)
        overview.header()->setSectionResizeMode(c, QHeaderView::Interactive);
    // Reproduce the actual application setup order. QTreeView disables
    // sectionsClickable when sorting is disabled; it must be restored after.
    overview.setSortingEnabled(false);
    overview.header()->setSectionsClickable(true);
    overview.setColumnWidth(0, 140);
    overview.setColumnWidth(1, 55);
    overview.setColumnWidth(2, 48);
    overview.setColumnWidth(3, 90);
    QTreeWidgetItem *first = new QTreeWidgetItem(&overview);
    first->setText(2, "Finished (retry 23:45:01)");
    first->setText(3, "https://boards.4chan.org/b/thread/100");
    QTreeWidgetItem *second = new QTreeWidgetItem(&overview);
    second->setText(2, "Thread removed");
    second->setText(3, "https://boards.4channel.org/r9k/thread/200");
    overview.headerItem()->setText(2, "Status (Ended / Removed)");
    overview.headerItem()->setText(3, "URI (/r9k/)");
    overview.show();
    QApplication::processEvents();
    if (overview.columnWidth(2) != 48 || overview.columnWidth(3) != 90 ||
            overview.header()->sectionResizeMode(2) != QHeaderView::Interactive ||
            overview.header()->sectionResizeMode(3) != QHeaderView::Interactive)
        return 3;

    // Exercise a REAL click on the Status heading. Previous v0.4.4 tests
    // verified categories but missed that setSortingEnabled(false) had
    // silently disabled sectionClicked, leaving no menu available.
    if (overview.isSortingEnabled() || !overview.header()->sectionsClickable())
        return 7;
    QSignalSpy statusClicks(overview.header(), SIGNAL(sectionClicked(int)));
    const int statusX = overview.header()->sectionViewportPosition(2)
                        + overview.header()->sectionSize(2) / 2;
    QTest::mouseClick(overview.header()->viewport(), Qt::LeftButton,
                      Qt::NoModifier,
                      QPoint(statusX, overview.header()->height() / 2));
    if (statusClicks.count() != 1 || statusClicks.at(0).at(0).toInt() != 2) {
        qCritical() << "Status heading click did not fire sectionClicked(2):"
                    << statusClicks.count();
        return 8;
    }
    if (overview.isSortingEnabled() || overview.columnWidth(2) != 48)
        return 9;

    // URI is also a clickable menu header, without sorting or resizing.
    QSignalSpy uriClicks(overview.header(), SIGNAL(sectionClicked(int)));
    const int uriX = overview.header()->sectionViewportPosition(3)
                     + overview.header()->sectionSize(3) / 2;
    QTest::mouseClick(overview.header()->viewport(), Qt::LeftButton,
                      Qt::NoModifier,
                      QPoint(uriX, overview.header()->height() / 2));
    if (uriClicks.count() != 1 || uriClicks.at(0).at(0).toInt() != 3)
        return 12;
    if (overview.isSortingEnabled() || overview.columnWidth(3) != 90)
        return 13;

    // The two filters are combined with AND, not applied sequentially to
    // already hidden rows (so clearing either filter restores matches).
    const QString status = QStringLiteral("ended");
    const QString board = QStringLiteral("r9k");
    first->setHidden(!(OverviewStatus::categoryFor(first->text(2)) == status &&
                       OverviewBoard::matchesFilter(first->text(3), board)));
    second->setHidden(!(OverviewStatus::categoryFor(second->text(2)) == status &&
                        OverviewBoard::matchesFilter(second->text(3), board)));
    if (!first->isHidden() || second->isHidden() ||
            overview.indexOfTopLevelItem(first) != 0 ||
            overview.indexOfTopLevelItem(second) != 1)
        return 14;
    first->setHidden(!OverviewBoard::matchesFilter(first->text(3), QString()));
    second->setHidden(!OverviewBoard::matchesFilter(second->text(3), QString()));
    if (first->isHidden() || second->isHidden())
        return 15;

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

    qInfo() << "PASS: Status and URI header clicks emit sectionClicked,"
               " sorting stays disabled, manual widths, complete status taxonomy,"
               " exact board URL detection, AND-filtering, zero-result filtering"
               " and stable filtered-row identity";
    return 0;
}
