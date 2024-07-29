#include "widget.h"
#include "ui_widget.h"
#include <windows.h>
#include <QDebug>
#include <QDir>
#include "folderIconSelector.h"
#include <QTimer>
#include <QLayout>
#include <QFileDialog>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->lw = ui->listWidget;
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowTitle("Dr.Folder - MrBeanC");

    // 创建状态栏
    statusBar = new QStatusBar(this);
    statusBar->setFixedHeight(15);
    this->layout()->addWidget(statusBar);
    statusBar->showMessage("Ready", 1000);

    lw->setAlternatingRowColors(true);

    ui->lineEdit->setReadOnly(true);

    connect(ui->btn_select, &QToolButton::clicked, this, [=](){
       QString path = QFileDialog::getExistingDirectory(this, "Select a folder");
       ui->lineEdit->setText(QDir::toNativeSeparators(path));
    });

    connect(ui->btn_search, &QToolButton::clicked, this, [=](){
        listSubDirs(ui->lineEdit->text());
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
