#ifndef WIDGET_H
#define WIDGET_H

#include <QListWidget>
#include <QWidget>

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

private:
    Ui::Widget *ui;

    QListWidget* lw = nullptr;
};
#endif // WIDGET_H
