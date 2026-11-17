#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QDebug>
#include "apptheme.h"

int main(int argc, char **argv) {
    // Run a real Qt Widgets test without requiring a Windows desktop session.
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
#ifdef Q_OS_WIN
    app.setStyle("windowsvista");
#else
    app.setStyle("fusion");
#endif

    const QPalette original = app.palette();
    const QString originalStyle = app.style()->objectName();
    const QString originalCss = app.styleSheet();

    // Existing installations and malformed settings must remain Light.
    AppTheme::applyTheme(QStringLiteral("light"));
    if (AppTheme::isDark() || app.palette() != original) return 1;

    QTabWidget tabs;
    QTreeWidget overview;
    QLabel label;
    tabs.addTab(&label, "Thread");
    overview.setColumnCount(4);

    AppTheme::applyTheme(QStringLiteral("dark"));
    if (!AppTheme::isDark()) return 2;
    const QPalette dark = app.palette();
    if (dark.color(QPalette::Window) != QColor("#202329")) return 3;
    if (dark.color(QPalette::Button) != QColor("#383e49")) return 4;
    if (dark.color(QPalette::WindowText) != QColor("#e6e9ee")) return 5;
    if (dark.color(QPalette::HighlightedText) != QColor("#ffffff")) return 6;
    if (dark.color(QPalette::Highlight) != QColor("#316da9")) return 7;
    if (app.styleSheet().isEmpty() ||
            !app.styleSheet().contains("QTabBar::tab:selected") ||
            !app.styleSheet().contains("QScrollBar:vertical")) return 8;
    if (AppTheme::overviewTextColor() != QColor("#e6e9ee") ||
            AppTheme::overviewNewImagesColor() != QColor("#78dcae") ||
            AppTheme::warningTextColor() != "#ffaca5") return 9;

    // Switching while widgets exist must restore original Light styling.
    AppTheme::applyTheme(QStringLiteral("light"));
    if (AppTheme::isDark()) return 10;
    if (app.palette() != original ||
            app.styleSheet() != originalCss ||
            app.style()->objectName() != originalStyle) {
        qWarning() << "Light restoration mismatch:"
                   << "palette equal=" << (app.palette() == original)
                   << "CSS equal=" << (app.styleSheet() == originalCss)
                   << "old style=" << originalStyle
                   << "restored style=" << app.style()->objectName()
                   << "original window=" << original.color(QPalette::Window)
                   << "restored window=" << app.palette().color(QPalette::Window);
        return 11;
    }
    if (AppTheme::overviewTextColor() != QColor(Qt::black) ||
            AppTheme::overviewNewImagesColor() != QColor(Qt::darkGreen) ||
            AppTheme::warningTextColor() != "#aa0000") return 12;

    AppTheme::applyTheme(QStringLiteral("DaRk"));
    if (!AppTheme::isDark()) return 13;
    AppTheme::applyTheme(QStringLiteral("unsupported"));
    if (AppTheme::isDark() || app.palette() != original) return 14;

    qInfo() << "PASS: Light default, charcoal palette, status contrast,"
               " live switching, native Light restoration and invalid preference fallback";
    return 0;
}
