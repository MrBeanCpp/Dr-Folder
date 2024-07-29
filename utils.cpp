#include "utils.h"
#include <QDebug>
#include <QFile>
#include <shlobj.h>
#include <shlwapi.h>
#include <QDirIterator>
#include <QFileIconProvider>
#include <QSettings>
#include <QTextCodec>

void Util::setFolderIcon(const QString &folderPath, const QString &iconPath, int index)
{
    SHFOLDERCUSTOMSETTINGS fcs = {0}; // 初始化所有成员为0
    fcs.dwSize = sizeof(SHFOLDERCUSTOMSETTINGS);
    fcs.dwMask = FCSM_ICONFILE;
    auto iconWStr = iconPath.toStdWString(); // IMPORTANT: 不能写为 iconPath.toStdWString().c_str()，因为返回的是临时对象，导致指针无效
    fcs.pszIconFile = LPWSTR(iconWStr.c_str());
    fcs.cchIconFile = 0;
    fcs.iIconIndex = index;

    // 这里返回临时对象指针没事，因为语句没结束不会被释放
    HRESULT hr = SHGetSetFolderCustomSettings(&fcs, folderPath.toStdWString().c_str(), FCS_FORCEWRITE);
    if (FAILED(hr)) {
        qWarning() << "Failed to set folder icon";
    }
}

QStringList Util::getExeFiles(const QString& dirPath) {
    QStringList res;
    QDir dir(dirPath);
    if (!dir.exists()) return res;

    dir.setNameFilters(QStringList() << "*.exe");
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    // 若根目录没有exe文件 & 只有一个子目录，则进入子目录查找exe文件 （只进一层）
    if (files.isEmpty()) {
        auto dirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot); // AllDirs 不受到 nameFilters 影响
        if (dirs.size() == 1) {
            dir.cd(dirs[0]);
            files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        }
    }

    // 获取绝对路径
    for (const auto& name : qAsConst(files)) {
        if (name.startsWith("unins", Qt::CaseInsensitive)) continue; // 忽略卸载程序
        res.push_back(dir.absoluteFilePath(name));
    }

    // 再搜索一下 bin 目录, 应该不会有bin/bin/bin吧，递归没事
    if (dir.cd("bin"))
        res.append(getExeFiles(dir.absolutePath()));
    return res;
}

// 重置文件夹图标
void Util::restoreFolderIcon(const QString& folderPath)
{
    QFile iniFile(folderPath + "/desktop.ini");
    if (!iniFile.exists()) return;

    // delete desktop.ini
    iniFile.remove();
    // remove attrib
    PathUnmakeSystemFolder(folderPath.toStdWString().c_str());

    // 很奇怪 设置图标的时候 PathMakeSystemFolder 不会刷新图标，但是Unmake会刷新
    // 而且 Unmake不会删除 desktop.ini
}

// 直接读取 desktop.ini 的话遇到编码问题
QString Util::getFolderIconPath(const QString& folderPath) {
    SHFILEINFO shFileInfo;
    ZeroMemory(&shFileInfo, sizeof(SHFILEINFO));

    // Convert QString to std::wstring
    // 必须要把分隔符转为windows的分隔符，否则会出现找不到文件的情况
    std::wstring folderPathW = QDir::toNativeSeparators(folderPath).toStdWString();

    // Get the icon location for the folder
    if (SHGetFileInfo(folderPathW.c_str(), 0, &shFileInfo, sizeof(SHFILEINFO), SHGFI_ICONLOCATION)) {
        return QString::fromWCharArray(shFileInfo.szDisplayName);
    } else {
        return QString();
    }
}

// Windows API很快，< 1ms，但是假如先获取图标再转为QImage判断，就很慢 > 40ms
bool Util::hasCustomIcon(const QString& exePath)
{
    HMODULE hModule = LoadLibraryEx(exePath.toStdWString().c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModule == NULL) {
        qWarning() << "Failed to load library:" << GetLastError() << exePath;
        return false;
    }

    bool hasIcon = false;

    auto enumProc = [](HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) -> BOOL {
        Q_UNUSED(hModule);
        Q_UNUSED(lpType);
        Q_UNUSED(lpName);

        bool* result = (bool*)(lParam);
        *result = true;
        return FALSE; // Stop enumeration after finding the first icon
    };

    // 也可以通过返回值判断，但是enumProc不能返回FALSE
    EnumResourceNames(hModule, RT_GROUP_ICON, enumProc, LONG_PTR(&hasIcon));

    FreeLibrary(hModule);
    return hasIcon;
}

