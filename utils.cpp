#include "utils.h"
#include <QDebug>
#include <QFile>
#include <shlobj.h>
#include <shlwapi.h>
#include <QDirIterator>
#include <QtWinExtras>
#include <QFileIconProvider>

void Util::setFolderIcon(const QString &folderPath, const QString &iconPath, int index)
{
    SHFOLDERCUSTOMSETTINGS fcs = {0}; // 初始化所有成员为0
    fcs.dwSize = sizeof(SHFOLDERCUSTOMSETTINGS);
    fcs.dwMask = FCSM_ICONFILE;
    auto iconWStr = QDir::toNativeSeparators(iconPath).toStdWString(); // IMPORTANT: 不能写为 iconPath.toStdWString().c_str()，因为返回的是临时对象，导致指针无效
    fcs.pszIconFile = LPWSTR(iconWStr.c_str());
    fcs.cchIconFile = 0;
    fcs.iIconIndex = index;

    // 这里返回临时对象指针没事，因为语句没结束不会被释放
    HRESULT hr = SHGetSetFolderCustomSettings(&fcs, QDir::toNativeSeparators(folderPath).toStdWString().c_str(), FCS_FORCEWRITE);
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

// 在没有缓存的情况下（重启），无论是什么方法，都不可避免要读取icon，都很慢
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
// slow
bool Util::isUsingDefaultIcon(const QString& exePath)
{
    UINT iconCount = ExtractIconEx(exePath.toStdWString().c_str(), -1, NULL, NULL, 0);
    return iconCount == 0;
}
// slow
bool Util::isDefaultExeIcon(const QIcon& icon) {
    static const QSize IconSize(16, 16);

    static auto defaultIcon = []() -> QIcon {
        QFileIconProvider iconProvider;
        return iconProvider.icon(QFileInfo("invalid_path_mrbeanc.exe"));
    }().pixmap(IconSize).toImage();

    return icon.pixmap(IconSize).toImage() == defaultIcon;
}

// QFileIconProvider::icon会缓存，导致无法更新图标，故采用 SHGetFileInfo
// 但是QFileIconProvider::icon优化更好，速度更快，所以只在必要的地方手动调用 SHGetFileInfo
// 其实QFileIconProvider::icon().pixmap()也是调用SHGetFileInfo
// .icon()其实并没有获取到图像，只是传入了IconEngine，真正获取图像是在.pixmap()时
// 更快的原因可能是采用了多线程获取pixmap，或者分区域绘制

// 通过源码可以发现，QFileIconProvider只会缓存Folder图标，不缓存文件图标
// 缓存方式为：QPixmapCache，所以调用clear()静态方法就可以清空
QIcon Util::getFileIcon(QString filePath) {
    filePath = QDir::toNativeSeparators(filePath); // IMPORTANT: 否则会找不到文件

    CoInitialize(NULL); // important for SHGetFileInfo，否则失败
    SHFILEINFO sfi;
    memset(&sfi, 0, sizeof(SHFILEINFO));

    QIcon icon;
    if (SHGetFileInfo(filePath.toStdWString().c_str(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON)) {
        icon = QtWin::fromHICON(sfi.hIcon);
        DestroyIcon(sfi.hIcon);
    }
    CoUninitialize();

    return icon;
}

// 对于绝对路径，QDir::exists() 不可靠（for判断是否inDir）
bool Util::isInDir(const QString& filePath, const QString& dirPath)
{
    auto _filePath = QDir::toNativeSeparators(filePath);
    auto _dirPath = QDir::toNativeSeparators(dirPath);
    return _filePath.startsWith(_dirPath);
}
