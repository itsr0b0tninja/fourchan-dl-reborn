#ifndef COMPONENTTREELAYOUT_H
#define COMPONENTTREELAYOUT_H

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHeaderView>
#include <QSet>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

namespace ComponentTreeLayout {

// A stable hierarchical key lets expansion survive a component-list rebuild.
// The dialog itself stays alive while hidden, so reopening also preserves
// expanded branches without any on-disk settings.
inline QString itemKey(const QTreeWidgetItem *item) {
    QStringList pieces;
    for (const QTreeWidgetItem *node = item; node; node = node->parent())
        pieces.prepend(node->text(0));
    return pieces.join(QChar(0x1f));
}

inline QSet<QString> expandedPaths(QTreeWidget *tree) {
    QSet<QString> paths;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        if ((*it)->isExpanded())
            paths.insert(itemKey(*it));
    }
    return paths;
}

inline void restoreExpandedPaths(QTreeWidget *tree, const QSet<QString> &paths) {
    for (QTreeWidgetItemIterator it(tree); *it; ++it)
        (*it)->setExpanded(paths.contains(itemKey(*it)));
}

inline void measure(const QTreeWidget *tree, const QTreeWidgetItem *item,
                    int depth, int *labelWidth, int *valueWidth) {
    // Include each nesting level, the branch expander and the text margins.
    // A plain resizeColumnToContents on a hidden QTreeWidget header can
    // underestimate the available indent area, truncating "Interface Revision".
    const QFontMetrics metrics(tree->font());
    const int indent = qMax(16, tree->indentation());
    const int requiredLabel = metrics.horizontalAdvance(item->text(0))
                            + (depth + 1) * indent + 36;
    *labelWidth = qMax(*labelWidth, requiredLabel);
    if (!item->text(1).isEmpty())
        *valueWidth = qMax(*valueWidth, metrics.horizontalAdvance(item->text(1)) + 28);
    for (int i = 0; i < item->childCount(); ++i)
        measure(tree, item->child(i), depth + 1, labelWidth, valueWidth);
}

inline void sizeColumns(QTreeWidget *tree) {
    if (!tree) return;
    QHeaderView *header = tree->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    int labelWidth = 160;
    int valueWidth = 120;
    for (int i = 0; i < tree->topLevelItemCount(); ++i)
        measure(tree, tree->topLevelItem(i), 0, &labelWidth, &valueWidth);

    // The first column must NEVER be shrunk just because the user picked a
    // narrow dialog. Preserve both columns and enable horizontal scrolling.
    const int usableWidth = tree->viewport()->width();
    tree->setColumnWidth(0, labelWidth);
    tree->setColumnWidth(1, qMax(valueWidth, usableWidth - labelWidth));
}

} // namespace ComponentTreeLayout

#endif // COMPONENTTREELAYOUT_H
