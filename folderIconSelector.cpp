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
        auto filePath = ui->comboBox->currentData().toString();
        if (filePath.isEmpty()) return;

        Util::setFolderIcon(dirPath, filePath);

        int selectedIndex = ui->comboBox->currentIndex();
        setIcon(ui->comboBox->itemIcon(selectedIndex));
    });

    setIcon(iconPro.icon(QFileInfo(dirPath)));

    // 异步，否则QCombobox的宽度会不太正常（不对齐）
    QTimer::singleShot(0, this, [=](){
        QDir dir(dirPath);
        ui->label->setText(dir.dirName());
        auto files = Util::getExeFiles(dirPath);
        // 展示可选exe图标
        ui->comboBox->setUpdatesEnabled(false);
        for (const QString& path : files) {
            if (!Util::hasCustomIcon(path)) continue; // 过滤没有自定义图标的exe
            QIcon icon = iconPro.icon(QFileInfo(path));
            ui->comboBox->addItem(icon, QFileInfo(path).fileName(), QDir::toNativeSeparators(path));
        }
        // 选中当前文件夹的图标
        auto iconPath = Util::getFolderIconPath(dirPath);
        if (!iconPath.isEmpty()) {
            int index = ui->comboBox->findData(iconPath, Qt::UserRole, Qt::MatchContains);
            if (index != -1)
                ui->comboBox->setCurrentIndex(index);
        }
        ui->comboBox->setUpdatesEnabled(true);
    });

}

FolderIconSelector::~FolderIconSelector()
{
    delete ui;
}

void FolderIconSelector::setIcon(const QIcon& icon)
{
    auto iconSize = ui->icon->size();
    auto iconPixmap = icon.pixmap(iconSize); // size可能比iconSize大，很奇怪，必须缩放
    if (iconSize != iconPixmap.size())
        iconPixmap = iconPixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->icon->setPixmap(iconPixmap);
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
    QAction *action_restore = menu.addAction("Restore Default Icon"); // del desktop.ini
    connect(action_restore, &QAction::triggered, this, [=]() {
        setIcon(iconPro.icon(QFileIconProvider::Folder));
        Util::restoreFolderIcon(dirPath);
    });
    menu.exec(event->globalPos());
}
