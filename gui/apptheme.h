#ifndef APPTHEME_H
#define APPTHEME_H

#include <QString>
#include <QColor>

namespace AppTheme {

// Two explicit colour palettes on the same built-in Qt Fusion control style.
void applyTheme(const QString &name);
bool isDark();
QColor overviewTextColor();
QColor overviewNewImagesColor();
QString warningTextColor();

} // namespace AppTheme

#endif // APPTHEME_H
