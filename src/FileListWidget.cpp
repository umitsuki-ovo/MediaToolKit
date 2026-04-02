#include "FileListWidget.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QUrl>
#include <QListWidgetItem>

FileListWidget::FileListWidget(QWidget* parent) : QListWidget(parent)
{
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAlternatingRowColors(true);

    connect(this, &QListWidget::currentRowChanged,
            this, &FileListWidget::onCurrentRowChanged);
}

void FileListWidget::addFiles(const QStringList& paths)
{
    int firstNew = m_items.size();
    for (const QString& p : paths) {
        QFileInfo fi(p);
        if (!fi.exists() || !fi.isFile()) continue;

        bool dup = false;
        for (const auto& it : m_items) {
            if (it.path == p) { dup = true; break; }
        }
        if (dup) continue;

        FileItem item = FileItem::fromPath(p);
        if (item.mediaType == MediaType::Unknown) continue;

        m_items.append(item);
        auto* wi = new QListWidgetItem(this);
        wi->setSizeHint(QSize(0, 42));
        refreshItem(m_items.size() - 1);
    }
    emit filesChanged();

    // D&D で追加した最初のファイルを選択
    if (m_items.size() > firstNew)
        setCurrentRow(firstNew);
}

void FileListWidget::removeSelected()
{
    QList<int> rows;
    for (auto* it : selectedItems())
        rows.prepend(row(it));
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) {
        delete takeItem(r);
        m_items.remove(r);
    }
    emit filesChanged();
}

void FileListWidget::clearAll()
{
    m_items.clear();
    clear();
    emit filesChanged();
}

void FileListWidget::setShowSizeInfo(bool show)
{
    if (m_showSizeInfo == show) return;
    m_showSizeInfo = show;
    for (int i = 0; i < m_items.size(); i++)
        refreshItem(i);
}

void FileListWidget::refreshItem(int index)
{
    if (index < 0 || index >= count()) return;
    const FileItem& fi = m_items[index];

    QString typeTag;
    switch (fi.mediaType) {
        case MediaType::Image: typeTag = "[画像]"; break;
        case MediaType::Video: typeTag = "[動画]"; break;
        case MediaType::Audio: typeTag = "[音声]"; break;
        default:               typeTag = "[  ?  ]"; break;
    }

    QListWidgetItem* wi = item(index);
    if (!wi) return;

    if (m_showSizeInfo) {
        wi->setText(QString("%1 %2\n   %3  |  %4")
            .arg(typeTag, fi.name, fi.sizeString(), fi.statusString()));
    } else {
        wi->setText(QString("%1 %2\n   %3")
            .arg(typeTag, fi.name, fi.statusString()));
    }

    switch (fi.status) {
        case FileStatus::Done:
            wi->setForeground(QColor("#4caf50")); break;
        case FileStatus::Error:
            wi->setForeground(QColor("#f44336")); break;
        case FileStatus::Processing:
            wi->setForeground(QColor("#2196f3")); break;
        default:
            wi->setForeground(QColor("#ccc")); break;
    }
}

void FileListWidget::updateItemStatus(int index, FileStatus status,
                                       const QString& err, qint64 outBytes,
                                       const QString& outputPath)
{
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].status    = status;
    m_items[index].errorMsg  = err;
    if (outBytes > 0)          m_items[index].outputBytes = outBytes;
    if (!outputPath.isEmpty()) m_items[index].outputPath  = outputPath;
    refreshItem(index);
}

void FileListWidget::resetAllStatus()
{
    for (int i = 0; i < m_items.size(); i++) {
        m_items[i].status      = FileStatus::Pending;
        m_items[i].errorMsg    = {};
        m_items[i].outputBytes = 0;
        m_items[i].outputPath  = {};
        refreshItem(i);
    }
}

void FileListWidget::onCurrentRowChanged(int row)
{
    if (row >= 0 && row < m_items.size())
        emit fileSelected(m_items[row]);
}

void FileListWidget::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void FileListWidget::dragMoveEvent(QDragMoveEvent* e)
{
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void FileListWidget::dropEvent(QDropEvent* e)
{
    QStringList paths;
    for (const QUrl& url : e->mimeData()->urls())
        if (url.isLocalFile()) paths << url.toLocalFile();
    if (!paths.isEmpty())
        addFiles(paths);  // addFiles 内で自動選択
    e->acceptProposedAction();
}
