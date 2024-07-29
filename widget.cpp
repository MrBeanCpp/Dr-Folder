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
    statusBar->showMessage("Ready", 1000);

    lw->setAlternatingRowColors(true);
    lw->hide();

    ui->placeholder->setCursor(Qt::PointingHandCursor);
    connect(ui->placeholder, &QPushButton::clicked, ui->btn_select, &QToolButton::click);

    connect(ui->btn_select, &QToolButton::clicked, this, [=](){
       QString path = QFileDialog::getExistingDirectory(this, "Select a folder");
       ui->lineEdit->setText(QDir::toNativeSeparators(path));
    });

    connect(ui->btn_subdir, &QToolButton::clicked, this, [=](){
        if (!beforeAddItems()) return;

        listSubDirs(ui->lineEdit->text());
    });

    connect(ui->btn_self, &QToolButton::clicked, this, [=](){
        if (!beforeAddItems()) return;

        addListItem(ui->lineEdit->text());
    });
}

Widget::~Widget()
{
    delete ui;
}

void Widget::addListItem(const QString& path)
{
    QListWidgetItem *item = new QListWidgetItem(lw);
    FolderIconSelector *customWidget = new FolderIconSelector(path);
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
        statusBar->showMessage(QString("Loading... %1/%2").arg(i).arg(dirs.size()), 1000);
        if (++i % 5 == 0)
            qApp->processEvents();
    }
    statusBar->showMessage("Done.", 2000);
}

bool Widget::beforeAddItems()
{
    QString path = ui->lineEdit->text();
    bool isOk = QFile::exists(path) && QFileInfo(path).isDir();

    if (isOk) {
        lw->clear();
        ui->placeholder->hide();
        lw->show();
    } else {
        statusBar->showMessage("Invalid folder path.", 2000);
    }

    return isOk;
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

    ui->btn_self->click(); // default Self
}
