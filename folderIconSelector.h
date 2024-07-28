#ifndef FOLDERICONSELECTOR_H
#define FOLDERICONSELECTOR_H

#include <QFileIconProvider>
#include <QWidget>

namespace Ui {
class FolderIconSelector;
}

class FolderIconSelector : public QWidget
{
    Q_OBJECT

public:
    explicit FolderIconSelector(const QString& dirPath, QWidget *parent = nullptr);
    ~FolderIconSelector();

private:
    // set Icon to Label
    void setIcon(const QIcon& icon);

private:
    Ui::FolderIconSelector *ui;

    const QString dirPath;
    static QFileIconProvider iconPro; // 获取图标是耗时操作，但是貌似会缓存

    // QObject interface
public:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

    // QWidget interface
protected:
    virtual void contextMenuEvent(QContextMenuEvent* event) override;
};

#endif // FOLDERICONSELECTOR_H
