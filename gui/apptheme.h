#ifndef APPTHEME_H
#define APPTHEME_H

#include <QString>
#include <QColor>

namespace AppTheme {

// Global Qt appearance; the original light application style/palette is
// captured before any dark styling and restored exactly when switching back.
void applyTheme(const QString &name);
bool isDark();
QColor overviewTextColor();
QColor overviewNewImagesColor();
QString warningTextColor();

} // namespace AppTheme

#endif // APPTHEME_H
