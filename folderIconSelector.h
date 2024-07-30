#ifndef FOLDERICONSELECTOR_H
#define FOLDERICONSELECTOR_H

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
    void openFolder();
    void addIconCandidate(const QString& path);

signals:
    void removeItems(QList<int> idxs);

private:
    Ui::FolderIconSelector *ui;

    const QString dirPath;

    // QObject interface
public:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

    // QWidget interface
protected:
    virtual void contextMenuEvent(QContextMenuEvent* event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
};

#endif // FOLDERICONSELECTOR_H
