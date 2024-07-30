#include "widget.h"
#include "ui_widget.h"
#include <windows.h>
#include <QDebug>
#include <QDir>
#include "folderIconSelector.h"
#include <QTimer>
#include <QLayout>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>
#include <QElapsedTimer>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->lw = ui->listWidget;
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowTitle("Dr.Folder - MrBeanC");
    this->setAcceptDrops(true);

    // 创建状态栏
    statusBar = new QStatusBar(this);
    statusBar->setFixedHeight(15);
    this->layout()->addWidget(statusBar);

    lw->setAlternatingRowColors(true);
    lw->hide();

    ui->placeholder->setCursor(Qt::PointingHandCursor);
    connect(ui->placeholder, &QPushButton::clicked, ui->btn_select, &QToolButton::click);

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [=]{
        ui->btn_subdir->click();
    });

    connect(ui->btn_select, &QToolButton::clicked, this, [=]{
       QString path = QFileDialog::getExistingDirectory(this, "Select a folder");
       if (path.isEmpty()) return;
       ui->lineEdit->setText(QDir::toNativeSeparators(path));
    });

    connect(ui->btn_subdir, &QToolButton::clicked, this, [=]{
        listFolders(ui->lineEdit->text());
    });

    ui->btn_subdir->setPopupMode(QToolButton::MenuButtonPopup);

    QMenu *menu = new QMenu(ui->btn_subdir);
    menu->addAction("Only Self", this, [=]{ listFolders(ui->lineEdit->text(), true); });

    ui->btn_subdir->setMenu(menu);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::addListItem(const QString& path)
{
    QListWidgetItem *item = new QListWidgetItem(lw);
    FolderIconSelector *customWidget = new FolderIconSelector(path, lw);
    item->setSizeHint(customWidget->sizeHint());
    lw->setItemWidget(item, customWidget);
}

void Widget::listSubDirs(const QString& dirPath)
{
    QDir dir(dirPath);
    auto dirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    int i = 0;
    for (const auto& name : dirs) {
        addListItem(dir.absoluteFilePath(name));
        statusBar->showMessage(QString("Loading... %1/%2: %3").arg(i).arg(dirs.size()).arg(name), 1000);
        if (++i % 2 == 0)
            qApp->processEvents();
    }
    statusBar->showMessage("Done.", 2000);
}

void Widget::listFolders(const QString& dirPath, bool onlySelf)
{
    beforeAddItems();
    if (onlySelf) {
        addListItem(dirPath);
    } else {
        listSubDirs(dirPath);
    }
}

bool Widget::beforeAddItems()
{
    QString path = ui->lineEdit->text();
    bool isOk = QFile::exists(path) && QFileInfo(path).isDir();

    if (isOk) {
        clearListItems();
        ui->placeholder->hide();
        lw->show();
    } else {
        statusBar->showMessage("Invalid folder path.", 2000);
    }

    return isOk;
}

void Widget::clearListItems()
{
    while (lw->count() > 0) {
        QListWidgetItem *item = lw->takeItem(0); // 移除但不删除项
        QWidget *widget = lw->itemWidget(item);
        if (widget) {
            delete widget; // 删除关联的子控件
        }
        delete item; // 删除列表项
    }
}

void Widget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void Widget::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    QString path = urls.first().toLocalFile();
    if (!QFileInfo(path).isDir()) {
        statusBar->showMessage("Not a folder.", 2000);
        return;
    }
    ui->lineEdit->setText(QDir::toNativeSeparators(path));

    listFolders(path, true); // default Self
}
