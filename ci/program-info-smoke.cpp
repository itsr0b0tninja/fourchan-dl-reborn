#include <QApplication>
#include <QDialog>
#include <QFontMetrics>
#include <QScrollBar>
#include <QTabWidget>
#include <QTreeWidget>
#include <QDebug>
#include "ui_uiinfo.h"
#include "componenttreelayout.h"

static QTreeWidgetItem *populate(QTreeWidget *tree) {
    QTreeWidgetItem *plugins = new QTreeWidgetItem(tree);
    plugins->setText(0, "Plugins");
    QTreeWidgetItem *plugin = new QTreeWidgetItem(plugins);
    plugin->setText(0, "4chan.org Parser");
    QTreeWidgetItem *author = new QTreeWidgetItem(plugin);
    author->setText(0, "Author");
    author->setText(1, "Mickey Fenton");
    QTreeWidgetItem *iface = new QTreeWidgetItem(plugin);
    iface->setText(0, "Interface Revision");
    iface->setText(1, "1.5");
    QTreeWidgetItem *formatting = new QTreeWidgetItem(plugin);
    formatting->setText(0, "Available formatting");
    QTreeWidgetItem *format = new QTreeWidgetItem(formatting);
    format->setText(0, "%n");
    format->setText(1, "Threadnumber");
    return plugin;
}

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setStyle("fusion");

    // Exercise the actual .ui rather than a synthetic replacement.
    QDialog dialog;
    Ui::UIInfo ui;
    ui.setupUi(&dialog);
    if (dialog.width() < 628 || dialog.height() < 372 ||
            ui.tabWidget->count() != 3 || ui.tabWidget->currentIndex() != 0 ||
            !ui.textEdit->isReadOnly() || !ui.label_2->openExternalLinks()) {
        qCritical() << "Program Information .ui layout or defaults regressed";
        return 1;
    }

    ui.tabWidget->setCurrentIndex(1);
    dialog.show();
    QApplication::processEvents();

    QTreeWidget *tree = ui.pluginInfo;
    QTreeWidgetItem *plugin = populate(tree);
    ComponentTreeLayout::sizeColumns(tree);
    QApplication::processEvents();
    const QFontMetrics font(tree->font());
    const int needed = font.horizontalAdvance(QStringLiteral("Available formatting"))
                     + 3 * qMax(16, tree->indentation()) + 36;
    if (tree->columnWidth(0) < needed || tree->header()->stretchLastSection()) {
        qCritical() << "Indented component fields still clipped" << tree->columnWidth(0)
                    << needed;
        return 2;
    }

    tree->topLevelItem(0)->setExpanded(true);
    plugin->setExpanded(true);
    // Closing and reopening the same dialog must keep expanded branches.
    dialog.hide();
    dialog.show();
    QApplication::processEvents();
    if (!tree->topLevelItem(0)->isExpanded() || !plugin->isExpanded()) {
        qCritical() << "Expanded groups were lost when reopening the dialog";
        return 3;
    }

    // Rebuilding the component tree also preserves expanded groups and plugins.
    const QSet<QString> expanded = ComponentTreeLayout::expandedPaths(tree);
    tree->clear();
    QTreeWidgetItem *rebuiltPlugin = populate(tree);
    ComponentTreeLayout::restoreExpandedPaths(tree, expanded);
    if (!tree->topLevelItem(0)->isExpanded() || !rebuiltPlugin->isExpanded()) {
        qCritical() << "Expanded groups were lost after refreshing components";
        return 4;
    }

    // A narrow window must scroll, not squeeze its first column into ellipses.
    QTreeWidget narrow;
    narrow.setColumnCount(2);
    populate(&narrow);
    narrow.resize(175, 200);
    narrow.show();
    QApplication::processEvents();
    ComponentTreeLayout::sizeColumns(&narrow);
    QApplication::processEvents();
    if (narrow.columnWidth(0) < needed ||
            narrow.horizontalScrollBar()->maximum() <= 0) {
        qCritical() << "Narrow component tree cannot scroll without clipping"
                    << narrow.columnWidth(0) << narrow.horizontalScrollBar()->maximum();
        return 5;
    }

    qInfo() << "PASS: Program Information defaults, complete indented labels,"
               " horizontal scroll, and expanded-tree session memory";
    return 0;
}
