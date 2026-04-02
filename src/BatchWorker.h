#pragma once
#include <QThread>
#include <QVector>
#include <atomic>
#include "FileItem.h"
#include "FFmpegProcessor.h"

class BatchWorker : public QThread
{
    Q_OBJECT
public:
    explicit BatchWorker(QObject* parent = nullptr);

    void setFiles(const QVector<FileItem>& files);
    void setCompressMode(bool compress);
    void setCompressOptions(const CompressOptions& opts);
    void setConvertOptions(const ConvertOptions& opts);
    void requestStop();
    bool isStopped() const { return m_stopRequested.load(); }

signals:
    void fileStarted(int index);
    void fileProgress(int index, float progress);
    void fileFinished(int index, bool ok, const QString& error,
                      qint64 outputBytes, const QString& outputPath);
    void allFinished();

protected:
    void run() override;

private:
    QVector<FileItem>     m_files;
    bool                  m_compressMode = true;
    CompressOptions       m_compOpts;
    ConvertOptions        m_convOpts;
    std::atomic<bool>     m_stopRequested{false};  // ★ atomic
};
