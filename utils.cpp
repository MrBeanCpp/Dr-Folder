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

QIcon Util::getSystemDefaultExeIcon() {
    // 获取一个已知的无效路径的图标，通常返回系统默认图标
    QFileIconProvider iconProvider;
    return iconProvider.icon(QFileInfo("invalid_path_mrbeanc.exe"));
}

bool Util::isDefaultExeIcon(const QIcon& icon) {
    const QSize IconSize(16, 16);
    static QIcon defaultIcon = getSystemDefaultExeIcon();
    // QIcon不能直接比较，而QPixmap本身没有定义operator==（调用的是QCursor的），所以需要转为QImage比较
    return icon.pixmap(IconSize).toImage() == defaultIcon.pixmap(IconSize).toImage();
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

