#pragma once
#include <QString>
#include <QFileInfo>
#include "MediaTypes.h"

enum class FileStatus { Pending, Processing, Done, Error, Skipped };

struct FileItem {
    QString  path;
    QString  name;
    qint64   sizeBytes   = 0;
    qint64   outputBytes = 0;   // 処理後サイズ（完了後に設定）
    QString  outputPath;        // 処理後ファイルパス（完了後に設定）
    MediaType mediaType  = MediaType::Unknown;
    FileStatus status    = FileStatus::Pending;
    QString  errorMsg;

    static FileItem fromPath(const QString& filePath)
    {
        FileItem item;
        item.path = filePath;
        QFileInfo fi(filePath);
        item.name       = fi.fileName();
        item.sizeBytes  = fi.size();
        item.mediaType  = getMediaType(fi.suffix());
        return item;
    }

    QString sizeString() const
    {
        auto fmt = [](qint64 b) -> QString {
            if (b < 1024)        return QString("%1 B").arg(b);
            if (b < 1024*1024)   return QString("%1 KB").arg(b/1024.0, 0,'f',1);
            return               QString("%1 MB").arg(b/1024.0/1024.0, 0,'f',2);
        };
        if (outputBytes > 0 && status == FileStatus::Done && sizeBytes > 0) {
            double reduction = 100.0 * (1.0 - (double)outputBytes / sizeBytes);
            QString tag = (reduction >= 0)
                ? QString("↓%1%").arg(reduction, 0, 'f', 1)
                : QString("↑%1%").arg(-reduction, 0, 'f', 1);
            return QString("%1 → %2 (%3)")
                .arg(fmt(sizeBytes), fmt(outputBytes), tag);
        }
        return fmt(sizeBytes);
    }

    QString statusString() const
    {
        switch (status) {
            case FileStatus::Pending:    return "待機中";
            case FileStatus::Processing: return "処理中...";
            case FileStatus::Done:       return "完了";
            case FileStatus::Error:      return "エラー: " + errorMsg;
            case FileStatus::Skipped:    return "スキップ";
        }
        return {};
    }
};
