#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <windows.h>
#include <QIcon>

namespace Util {
void setFolderIcon(const QString &folderPath, const QString &iconPath, int index = 0);
void restoreFolderIcon(const QString &folderPath);
QStringList getExeFiles(const QString& dirPath);
QString getFolderIconPath(const QString& folderPath);
// 判断exe是否有自定义图标
bool hasCustomIcon(const QString& exePath);
}

#endif // UTILS_H
