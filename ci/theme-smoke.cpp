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
        QFile output("../portable/theme-smoke-diagnostics.txt");
        if (output.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream out(&output);
            out << "palette_equal=" << (app.palette() == original)
                << " css_equal=" << (app.styleSheet() == originalCss)
                << " original_style=" << originalStyle
                << " restored_style=" << app.style()->objectName() << "\n";
            for (int g = QPalette::Active; g <= QPalette::Inactive; ++g) {
                const QPalette::ColorGroup group = static_cast<QPalette::ColorGroup>(g);
                for (int k = 0; k < QPalette::NColorRoles; ++k) {
                    const QPalette::ColorRole role = static_cast<QPalette::ColorRole>(k);
                    if (app.palette().brush(group, role) != original.brush(group, role)) {
                        out << "mismatch group=" << g << " role=" << k
                            << " old=" << original.color(group, role).name()
                            << " restored=" << app.palette().color(group, role).name()
                            << "\n";
                    }
                }
            }
            out.flush();
        }
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
