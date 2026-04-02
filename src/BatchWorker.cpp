#include "BatchWorker.h"
#include <QFileInfo>
#include <QDir>

BatchWorker::BatchWorker(QObject* parent) : QThread(parent) {}

void BatchWorker::setFiles(const QVector<FileItem>& files) { m_files = files; }
void BatchWorker::setCompressMode(bool compress)           { m_compressMode = compress; }
void BatchWorker::setCompressOptions(const CompressOptions& opts) { m_compOpts = opts; }
void BatchWorker::setConvertOptions(const ConvertOptions& opts)   { m_convOpts = opts; }
void BatchWorker::requestStop() { m_stopRequested.store(true); }

void BatchWorker::run()
{
    m_stopRequested.store(false);
    FFmpegProcessor proc;

    for (int i = 0; i < m_files.size(); i++) {
        if (m_stopRequested.load()) break;

        const FileItem& fi = m_files[i];
        emit fileStarted(i);

        // ★ progressCb 内で停止フラグを確認し、停止されたら false を返す
        auto progressCb = [this, i](float p) -> bool {
            emit fileProgress(i, p);
            return !m_stopRequested.load();  // false を返すと処理中断
        };

        QString err;
        if (m_compressMode)
            err = proc.compress(fi.path, m_compOpts, progressCb);
        else
            err = proc.convert(fi.path, m_convOpts, progressCb);

        // 停止要求でキャンセルされた場合
        if (m_stopRequested.load()) {
            emit fileFinished(i, false, "キャンセルされました", 0, {});
            break;
        }

        // 出力パスを推定
        QFileInfo info(fi.path);
        QString outDir = m_compressMode ? m_compOpts.outputDir : m_convOpts.outputDir;
        if (outDir.isEmpty()) outDir = info.absolutePath();

        QString outPath;
        if (m_compressMode) {
            outPath = QDir(outDir).filePath(
                info.completeBaseName() + "_compressed." + info.suffix());
        } else {
            QString newExt = m_convOpts.format;
            outPath = QDir(outDir).filePath(
                info.completeBaseName() + "." +
                (newExt.isEmpty() ? info.suffix() : newExt));
        }

        qint64 outBytes = err.isEmpty() ? QFileInfo(outPath).size() : 0;
        emit fileFinished(i, err.isEmpty(), err, outBytes, outPath);
    }

    emit allFinished();
}
