#ifndef WIDGET_H
#define WIDGET_H

#include <QListWidget>
#include <QWidget>
#include <QStatusBar>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    void addListItem(const QString& path);
    void listSubDirs(const QString& dirPath);

private:
    Ui::Widget *ui;

    QListWidget* lw = nullptr;
    QStatusBar *statusBar = nullptr;
};
#endif // WIDGET_H
