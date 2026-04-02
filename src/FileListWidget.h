#pragma once
#include <QListWidget>
#include <QVector>
#include "FileItem.h"

class FileListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit FileListWidget(QWidget* parent = nullptr);

    void addFiles(const QStringList& paths);
    void removeSelected();
    void clearAll();
    const QVector<FileItem>& fileItems() const { return m_items; }
    void updateItemStatus(int index, FileStatus status, const QString& err = {},
                          qint64 outBytes = 0, const QString& outputPath = {});
    void resetAllStatus();

    // 変換モード時はサイズ・圧縮率を非表示
    void setShowSizeInfo(bool show);

signals:
    void fileSelected(const FileItem& item);
    void filesChanged();

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private slots:
    void onCurrentRowChanged(int row);

private:
    void refreshItem(int index);
    QVector<FileItem> m_items;
    bool              m_showSizeInfo = true;
};
