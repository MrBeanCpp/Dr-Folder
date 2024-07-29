#include "widget.h"
#include "ui_widget.h"
#include <windows.h>
#include <QDebug>
#include <QDir>
#include "folderIconSelector.h"
#include <QStatusBar>
#include <QTimer>
#include <QLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->lw = ui->listWidget;
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);

    // 创建状态栏
    // QStatusBar *statusBar = new QStatusBar(this);
    // this->layout()->addWidget(statusBar);
    // statusBar->showMessage("Ready");

    lw->setAlternatingRowColors(true);

    QTimer::singleShot(100, this, [=](){
        QDir dir(R"(D:\)");
        auto dirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        int i = 0;
        for (const auto& name : dirs) {
            addListItem(dir.absoluteFilePath(name));
            if (++i % 5 == 0)
                qApp->processEvents();
        }
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
