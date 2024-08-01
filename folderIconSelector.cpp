#include "folderIconSelector.h"
#include "ui_folderIconSelector.h"
#include <QDir>
#include "utils.h"
#include <QDebug>
#include <QListView>
#include <QTimer>
#include <QContextMenuEvent>
#include <QMenu>
#include <shlwapi.h>
#include <QElapsedTimer>
#include <QDesktopServices>
#include <QtConcurrent>
#include <QFileDialog>
#include <QPixmapCache>
#include <QMessageBox>

QFileIconProvider FolderIconSelector::iconPro;
FolderIconSelector::FolderIconSelector(const QString& dirPath, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FolderIconSelector)
    , dirPath(dirPath)
{
    ui->setupUi(this);
    // 不加这句的话，无法设置item高度（原因未知）
    // https://blog.csdn.net/lilongwei_123/article/details/109503886
    ui->comboBox->setView(new QListView());

    ui->comboBox->installEventFilter(this);

    connect(ui->btn_apply, &QPushButton::clicked, this, [=](){
        auto iconPath = ui->comboBox->currentData().toString();
        if (iconPath.isEmpty()) return;

        if (Util::isInDir(iconPath, dirPath)) // 若icon在folder内，则转为相对路径
            iconPath = QDir(dirPath).relativeFilePath(iconPath);

        qDebug() << "Folder:" << dirPath << "Icon:" << iconPath;
        if (Util::setFolderIcon(dirPath, iconPath)) {
            QPixmapCache::clear(); // 清除缓存，否则文件夹图标不会更新
            setIcon(iconPro.icon(dirPath));
        } else {
            showActionFailed();
            // 貌似在不重启的情况下，无法UAC提权
            QMessageBox::warning(this, "Failed", "Failed to set folder icon.\nTry restart as [Administrator].");
        }
    });

    connect(ui->btn_select, &QToolButton::clicked, this, [=]{
       auto iconPath = QFileDialog::getOpenFileName(this, "Select an icon", dirPath, "Icon Files (*.ico *.exe)");
       if (iconPath.isEmpty()) return;
       addIconCandidate(iconPath);
       int index = ui->comboBox->count() - 1;
       ui->comboBox->setCurrentIndex(index);
    });

    setIcon(iconPro.icon(dirPath));

    // 异步，否则QCombobox的宽度会不太正常（不对齐）
    QTimer::singleShot(0, this, [=](){
        QDir dir(dirPath);
        ui->label->setText(dir.isRoot() ? dirPath : dir.dirName());
        auto files = Util::getExeFiles(dirPath);
        // 展示可选exe图标
        ui->comboBox->setUpdatesEnabled(false);
        for (const QString& path : files)
            addIconCandidate(path);

        // 选中当前文件夹的图标
        auto iconPath = Util::getFolderIconPath(dirPath);
        iconPath = QDir::toNativeSeparators(iconPath);

        if (!iconPath.isEmpty()) {
            int index = ui->comboBox->findData(iconPath, Qt::UserRole, Qt::MatchFixedString); // CaseInsensitive
            if (index != -1)
                ui->comboBox->setCurrentIndex(index);
        }
        ui->comboBox->setUpdatesEnabled(true);

        ui->comboBox->setEnabled(false);
        ui->btn_apply->setEnabled(false);
        // 过滤没有自定义图标的exe
        QtConcurrent::run([=](){
            QList<int> idxs;
            for (int i = 0; i < ui->comboBox->count(); ++i) {
                auto filePath = ui->comboBox->itemData(i).toString();
                if (!Util::hasCustomIcon(filePath)) // 耗时操作，必须放在子线程
                    idxs << i;
            }
            emit removeItems(idxs); // GUI操作必须放在主线程，通过信号与槽传递
        });
    });

    qRegisterMetaType<QList<int>>("QList<int>"); // for signal
    connect(this, &FolderIconSelector::removeItems, this, [=](QList<int> idxs){
        for (int i = idxs.size() - 1; i >= 0; --i) {
            // qDebug() << "clear default icon:" << ui->comboBox->itemText(idxs[i]);
            ui->comboBox->removeItem(idxs[i]);
        }
        ui->comboBox->setEnabled(true);
        ui->btn_apply->setEnabled(true);
    });
}

FolderIconSelector::~FolderIconSelector()
{
    delete ui;
}

void FolderIconSelector::applySelectedIcon()
{
    ui->btn_apply->click();
}

void FolderIconSelector::setIcon(const QIcon& icon)
{
    auto iconSize = ui->icon->size();
    auto iconPixmap = icon.pixmap(iconSize); // size可能比iconSize大，很奇怪，必须缩放
    if (iconSize != iconPixmap.size())
        iconPixmap = iconPixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->icon->setPixmap(iconPixmap);
}

void FolderIconSelector::openFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
}

void FolderIconSelector::addIconCandidate(const QString& path)
{
    auto icon = iconPro.icon(path);
    auto filename = QFileInfo(path).fileName();
    ui->comboBox->addItem(icon, filename, QDir::toNativeSeparators(path));
}

void FolderIconSelector::showActionFailed()
{
    ui->label->setStyleSheet("QLabel { color: red; background-color: rgba(255,255,0,60); font-weight: bold; border-radius: 6px; }");
}

bool FolderIconSelector::eventFilter(QObject* obj, QEvent* event)
{
    if(obj == ui->comboBox) {
        if (event->type() == QEvent::Wheel) // 拦截滚轮事件
            return true;
    }
    return QWidget::eventFilter(obj, event);
}

void FolderIconSelector::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction("Open Folder", this, [=]{
        openFolder();
    });
    menu.addAction("Restore Default Icon", this, [=]{  // del desktop.ini
        if (Util::restoreFolderIcon(dirPath)) {
            QPixmapCache::clear(); // 清除缓存，否则文件夹图标不会更新
            setIcon(iconPro.icon(dirPath));
        } else {
            showActionFailed();
            QMessageBox::warning(this, "Failed", "Failed to restore default icon.\nTry restart as [Administrator].");
        }
    });
    menu.exec(event->globalPos());
}

void FolderIconSelector::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        openFolder();
    QWidget::mouseDoubleClickEvent(event);
}
