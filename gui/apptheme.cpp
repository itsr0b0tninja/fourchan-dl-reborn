#include "apptheme.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

namespace {

bool initialized = false;
bool darkActive = false;
QPalette originalPalette;
QString originalStyleName;
QString originalStyleSheet;

void captureLightAppearance() {
    if (initialized)
        return;
    QApplication *app = qobject_cast<QApplication *>(QApplication::instance());
    if (!app)
        return;
    originalPalette = app->palette();
    originalStyleName = app->style()->objectName();
    originalStyleSheet = app->styleSheet();
    initialized = true;
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
    captureLightAppearance();

    const bool wantDark = name.trimmed().compare(QStringLiteral("dark"),
                                                 Qt::CaseInsensitive) == 0;
    if (wantDark == darkActive)
        return;

    if (wantDark) {
        // Native Windows controls ignore parts of the dark palette; Fusion
        // honours it consistently throughout the application's widgets.
        app->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        app->setPalette(charcoalPalette());
        app->setStyleSheet(charcoalStyleSheet());
    } else {
        QStyle *lightStyle = QStyleFactory::create(originalStyleName);
        if (lightStyle)
            app->setStyle(lightStyle);
        app->setPalette(originalPalette);
        app->setStyleSheet(originalStyleSheet);
    }

    darkActive = wantDark;
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
