#include "apptheme.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

namespace {

bool themeApplied = false;
bool darkActive = false;

// Both themes now use the same built-in Qt Fusion controls. No Windows Vista
// style plugin is needed, and both palettes are fully explicit.
QPalette lightPalette() {
    const QColor window("#f0f2f5");
    const QColor control("#e6eaf0");
    const QColor foreground("#202934");
    const QColor subdued("#707985");
    const QColor selection("#3175b8");

    QPalette palette;
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, foreground);
    palette.setColor(QPalette::Base, QColor("#ffffff"));
    palette.setColor(QPalette::AlternateBase, QColor("#f5f7fa"));
    palette.setColor(QPalette::Text, foreground);
    palette.setColor(QPalette::Button, control);
    palette.setColor(QPalette::ButtonText, foreground);
    palette.setColor(QPalette::BrightText, QColor("#ffffff"));
    palette.setColor(QPalette::ToolTipBase, QColor("#ffffff"));
    palette.setColor(QPalette::ToolTipText, foreground);
    palette.setColor(QPalette::Highlight, selection);
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::Light, QColor("#ffffff"));
    palette.setColor(QPalette::Midlight, QColor("#f5f7fa"));
    palette.setColor(QPalette::Mid, QColor("#c4cad4"));
    palette.setColor(QPalette::Dark, QColor("#929aa7"));
    palette.setColor(QPalette::Shadow, QColor("#4a535d"));
    palette.setColor(QPalette::Link, QColor("#1965ae"));
    palette.setColor(QPalette::LinkVisited, QColor("#7547ad"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    palette.setColor(QPalette::PlaceholderText, subdued);
#endif
    palette.setColor(QPalette::Disabled, QPalette::WindowText, subdued);
    palette.setColor(QPalette::Disabled, QPalette::Text, subdued);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, subdued);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#bac4d0"));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, foreground);
    return palette;
}

QPalette charcoalPalette() {
    const QColor window("#202329");
    const QColor panel("#2c3038");
    const QColor control("#383e49");
    const QColor text("#e6e9ee");
    const QColor subdued("#aab3bf");
    const QColor selection("#316da9"); // Contrast-safe selection behind white text.

    QPalette palette;
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, panel);
    palette.setColor(QPalette::AlternateBase, QColor("#323842"));
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, control);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, QColor("#ffffff"));
    palette.setColor(QPalette::ToolTipBase, control);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Highlight, selection);
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::Light, QColor("#525966"));
    palette.setColor(QPalette::Midlight, QColor("#424854"));
    palette.setColor(QPalette::Mid, QColor("#303540"));
    palette.setColor(QPalette::Dark, QColor("#16191e"));
    palette.setColor(QPalette::Shadow, QColor("#101217"));
    palette.setColor(QPalette::Link, QColor("#80bfff"));
    palette.setColor(QPalette::LinkVisited, QColor("#bdaaff"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    palette.setColor(QPalette::PlaceholderText, subdued);
#endif
    palette.setColor(QPalette::Disabled, QPalette::WindowText, subdued);
    palette.setColor(QPalette::Disabled, QPalette::Text, subdued);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, subdued);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#414958"));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, subdued);
    return palette;
}

QString lightStyleSheet() {
    // Match the Dark theme's widget geometry and selected-state design.
    // Only the colours differ; the underlying Qt control style is Fusion.
    return QStringLiteral(
        "QToolTip { color:#202934; background-color:#ffffff;"
        " border:1px solid #b6bdc8; padding:4px; }"
        "QMenu { background-color:#ffffff; color:#202934;"
        " border:1px solid #c4cad4; }"
        "QMenu::item:selected { background-color:#3175b8; color:#ffffff; }"
        "QTabWidget::pane { border:1px solid #c4cad4; }"
        "QTabBar::tab { background:#e6eaf0; color:#202934;"
        " border:1px solid #c4cad4; padding:5px 8px; }"
        "QTabBar::tab:selected { background:#ffffff;"
        " border-color:#3175b8; color:#202934; }"
        "QTabBar::tab:hover:!selected { background:#d5deea; }"
        "QTabBar::tab:disabled { color:#707985; }"
        "QDockWidget::title { background:#e6eaf0; color:#202934; padding:5px; }"
        "QHeaderView::section { background:#e6eaf0; color:#202934;"
        " border:1px solid #c4cad4; padding:4px; }"
        "QTreeView::item:selected, QListView::item:selected,"
        " QTableView::item:selected { background:#3175b8; color:#ffffff; }"
        "QTreeView::item:hover:!selected, QListView::item:hover:!selected {"
        " background:#e5edf7; }"
        "QProgressBar { color:#202934; background:#f5f7fa;"
        " border:1px solid #c4cad4; border-radius:3px; text-align:center; }"
        "QProgressBar::chunk { background:#3175b8; border-radius:2px; }"
        "QScrollBar:vertical { background:#edf0f4; width:12px; margin:0; }"
        "QScrollBar:horizontal { background:#edf0f4; height:12px; margin:0; }"
        "QScrollBar::handle:vertical { background:#a4adba; min-height:20px;"
        " margin:2px; border-radius:3px; }"
        "QScrollBar::handle:horizontal { background:#a4adba; min-width:20px;"
        " margin:2px; border-radius:3px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0;"
        " border:0; background:transparent; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }"
    );
}

QString charcoalStyleSheet() {
    // Fusion and the palette style the standard controls. Limit QSS to
    // elements whose native colour/selection behaviour is unreliable,
    // leaving icon rendering and normal widget interactions intact.
    return QStringLiteral(
        "QToolTip { color:#e6e9ee; background-color:#383e49;"
        " border:1px solid #626b79; padding:4px; }"
        "QMenu { background-color:#2c3038; color:#e6e9ee;"
        " border:1px solid #464d59; }"
        "QMenu::item:selected { background-color:#316da9; color:#ffffff; }"
        "QTabWidget::pane { border:1px solid #464d59; }"
        "QTabBar::tab { background:#2c3038; color:#e6e9ee;"
        " border:1px solid #464d59; padding:5px 8px; }"
        "QTabBar::tab:selected { background:#383e49;"
        " border-color:#4c93db; color:#ffffff; }"
        "QTabBar::tab:hover:!selected { background:#414958; }"
        "QTabBar::tab:disabled { color:#aab3bf; }"
        "QDockWidget::title { background:#2c3038; color:#e6e9ee; padding:5px; }"
        "QHeaderView::section { background:#383e49; color:#e6e9ee;"
        " border:1px solid #464d59; padding:4px; }"
        "QTreeView::item:selected, QListView::item:selected,"
        " QTableView::item:selected { background:#316da9; color:#ffffff; }"
        "QTreeView::item:hover:!selected, QListView::item:hover:!selected {"
        " background:#39414d; }"
        "QProgressBar { color:#e6e9ee; background:#2c3038;"
        " border:1px solid #464d59; border-radius:3px; text-align:center; }"
        "QProgressBar::chunk { background:#4c93db; border-radius:2px; }"
        "QScrollBar:vertical { background:#252a31; width:12px; margin:0; }"
        "QScrollBar:horizontal { background:#252a31; height:12px; margin:0; }"
        "QScrollBar::handle:vertical { background:#596371; min-height:20px;"
        " margin:2px; border-radius:3px; }"
        "QScrollBar::handle:horizontal { background:#596371; min-width:20px;"
        " margin:2px; border-radius:3px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0;"
        " border:0; background:transparent; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }"
    );
}

} // namespace

namespace AppTheme {

void applyTheme(const QString &name) {
    QApplication *app = qobject_cast<QApplication *>(QApplication::instance());
    if (!app)
        return;

    const bool wantDark = name.trimmed().compare(QStringLiteral("dark"),
                                                 Qt::CaseInsensitive) == 0;
    if (themeApplied && wantDark == darkActive)
        return;

    // Qt Fusion is built into Qt Widgets, so the Windows Vista DLL is no
    // longer needed. Use precisely the same control shapes in both modes.
    if (app->style()->objectName().compare(QStringLiteral("fusion"),
                                           Qt::CaseInsensitive) != 0) {
        QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion"));
        if (fusion)
            app->setStyle(fusion);
    }

    // Clear the old theme stylesheet before changing palettes. Qt's
    // stylesheet style can otherwise retain previous link colours.
    app->setStyleSheet(QString());
    app->setPalette(wantDark ? charcoalPalette() : lightPalette());
    app->setStyleSheet(wantDark ? charcoalStyleSheet() : lightStyleSheet());
    darkActive = wantDark;
    themeApplied = true;
}

bool isDark() {
    return darkActive;
}

QColor overviewTextColor() {
    return darkActive ? QColor("#e6e9ee") : QColor(Qt::black);
}

QColor overviewNewImagesColor() {
    return darkActive ? QColor("#78dcae") : QColor(Qt::darkGreen);
}

QString warningTextColor() {
    return darkActive ? QStringLiteral("#ffaca5")
                      : QStringLiteral("#aa0000");
}

} // namespace AppTheme
