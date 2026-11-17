#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "apptheme.h"

int main(int argc, char **argv) {
    // Run Qt Widgets without a Windows desktop session.
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setStyle("fusion");
    // QApplication::setStyleSheet wraps the Fusion style in Qt's
    // QStyleSheetStyle, which has a different objectName(). The source sets
    // Fusion; test its widgets/palette rather than the wrapper's name.
    if (app.style()->objectName().compare("fusion", Qt::CaseInsensitive) != 0)
        return 99;
    const auto fail = [&app](int code) -> int {
        QFile file("../portable/theme-smoke-diagnostics.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream out(&file);
            const QPalette p = app.palette();
            out << "theme-smoke failure=" << code
                << " style=" << app.style()->objectName()
                << " css_size=" << app.styleSheet().size() << "\n"
                << "Window=" << p.color(QPalette::Window).name()
                << " Base=" << p.color(QPalette::Base).name()
                << " Button=" << p.color(QPalette::Button).name()
                << " WindowText=" << p.color(QPalette::WindowText).name()
                << " Highlight=" << p.color(QPalette::Highlight).name()
                << " HighlightedText=" << p.color(QPalette::HighlightedText).name()
                << " Link=" << p.color(QPalette::Link).name()
                << " LinkVisited=" << p.color(QPalette::LinkVisited).name()
                << "\n";
        }
        return code;
    };

    QTabWidget tabs;
    QTreeWidget overview;
    QLabel label;
    tabs.addTab(&label, "Thread");
    overview.setColumnCount(4);

    // Light is the default for existing users and starts with Fusion controls.
    AppTheme::applyTheme(QStringLiteral("light"));
    if (AppTheme::isDark()) return fail(1);
    const QPalette light = app.palette();
    const QString lightCss = app.styleSheet();
    if (light.color(QPalette::Window) != QColor("#f0f2f5")) return fail(3);
    if (light.color(QPalette::Base) != QColor("#ffffff")) return fail(4);
    if (light.color(QPalette::Button) != QColor("#e6eaf0")) return fail(5);
    if (light.color(QPalette::WindowText) != QColor("#202934")) return fail(6);
    if (light.color(QPalette::Highlight) != QColor("#3175b8")) return fail(7);
    if (light.color(QPalette::HighlightedText) != QColor("#ffffff")) return fail(8);
    if (lightCss.isEmpty() || !lightCss.contains("QTabBar::tab:selected") ||
            !lightCss.contains("QScrollBar:vertical") ||
            !lightCss.contains("QProgressBar::chunk")) return fail(9);
    if (AppTheme::overviewTextColor() != QColor(Qt::black) ||
            AppTheme::overviewNewImagesColor() != QColor(Qt::darkGreen) ||
            AppTheme::warningTextColor() != "#aa0000") return fail(10);

    AppTheme::applyTheme(QStringLiteral("dark"));
    if (!AppTheme::isDark()) return fail(11);
    const QPalette dark = app.palette();
    const QString darkCss = app.styleSheet();
    if (dark.color(QPalette::Window) != QColor("#202329")) return fail(13);
    if (dark.color(QPalette::Button) != QColor("#383e49")) return fail(14);
    if (dark.color(QPalette::WindowText) != QColor("#e6e9ee")) return fail(15);
    if (dark.color(QPalette::Highlight) != QColor("#316da9")) return fail(16);
    if (dark.color(QPalette::HighlightedText) != QColor("#ffffff")) return fail(17);
    if (!darkCss.contains("QTabBar::tab:selected") ||
            !darkCss.contains("QScrollBar:vertical") ||
            !darkCss.contains("QProgressBar::chunk")) return fail(18);
    if (AppTheme::overviewTextColor() != QColor("#e6e9ee") ||
            AppTheme::overviewNewImagesColor() != QColor("#78dcae") ||
            AppTheme::warningTextColor() != "#ffaca5") return fail(19);

    // Applying Light while existing widget instances are alive must preserve
    // the full Light palette including all hyperlink/disabled colour roles.
    AppTheme::applyTheme(QStringLiteral("light"));
    if (AppTheme::isDark()) return fail(20);
    if (app.palette() != light || app.styleSheet() != lightCss) {
        qWarning() << "Light mismatch: palette=" << (app.palette() == light)
                   << "stylesheet=" << (app.styleSheet() == lightCss)
                   << "light link=" << light.color(QPalette::Link)
                   << "current link=" << app.palette().color(QPalette::Link)
                   << "light visited=" << light.color(QPalette::LinkVisited)
                   << "current visited=" << app.palette().color(QPalette::LinkVisited);
        return fail(22);
    }

    AppTheme::applyTheme(QStringLiteral("DaRk"));
    if (!AppTheme::isDark() || app.palette() != dark || app.styleSheet() != darkCss)
        return fail(23);
    AppTheme::applyTheme(QStringLiteral("unsupported"));
    if (AppTheme::isDark() || app.palette() != light || app.styleSheet() != lightCss)
        return fail(24);
    qInfo() << "PASS: Qt Fusion in both themes, unchanged Dark palette,"
               " Light defaults, selected controls, live switching and restore";
    return 0;
}
