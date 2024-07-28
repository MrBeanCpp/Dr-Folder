#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <windows.h>
#include <QIcon>

namespace Util {
void setFolderIcon(const QString &folderPath, const QString &iconPath, int index = 0);
void restoreFolderIcon(const QString &folderPath);
QStringList getExeFiles(const QString& dirPath);
QIcon getSystemDefaultExeIcon();
bool isDefaultExeIcon(const QIcon& icon);
QString getFolderIconPath(const QString& folderPath);
}

#endif // UTILS_H
