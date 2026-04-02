#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QAction>
#include <QStatusBar>
#include <QTimer>
#include <QElapsedTimer>
#include "FileListWidget.h"
#include "CompressWidget.h"
#include "ConvertWidget.h"
#include "PreviewWidget.h"
#include "BatchWorker.h"
#include "SettingsDialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onAddFiles();
    void onRemoveFiles();
    void onClearFiles();
    void onSwitchCompress();   // 圧縮モードに切り替え
    void onSwitchConvert();    // 変換モードに切り替え
    void onStartProcess();
    void onStopProcess();
    void onFileSelected(const FileItem& item);
    void onFilesChanged();
    void onOpenSettings();
    void onOpenOutputFolder();

    // BatchWorker
    void onFileStarted(int index);
    void onFileProgress(int index, float progress);
    void onFileFinished(int index, bool ok, const QString& error,
                        qint64 outputBytes, const QString& outputPath);
    void onAllFinished();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void applyMode(bool compress);   // compress=true: 圧縮, false: 変換
    void updateStartButton();
    void setProcessing(bool processing);
    void applySettingsToWidgets();

    // ---- ウィジェット ----
    QSplitter*      m_mainSplitter   = nullptr;

    // 左: ファイル一覧
    FileListWidget* m_fileList       = nullptr;
    QLabel*         m_statusLabel    = nullptr;

    // 右: 設定スタック + プレビュー
    QSplitter*      m_rightSplitter  = nullptr;
    QStackedWidget* m_settingsStack  = nullptr;
    CompressWidget* m_compressWidget = nullptr;
    ConvertWidget*  m_convertWidget  = nullptr;
    QWidget*        m_previewBox     = nullptr;
    PreviewWidget*  m_previewWidget  = nullptr;

    // 下部
    QPushButton*  m_startBtn      = nullptr;
    QPushButton*  m_stopBtn       = nullptr;
    QProgressBar* m_totalProgress = nullptr;

    // ツールバーの圧縮/変換ボタン (QWidgetAction に埋め込む)
    QPushButton*  m_tbCompressBtn = nullptr;
    QPushButton*  m_tbConvertBtn  = nullptr;

    // バッチ処理
    BatchWorker*  m_worker        = nullptr;
    int           m_totalFiles    = 0;
    int           m_doneFiles     = 0;
    QString       m_lastOutputDir;
    bool          m_compressMode  = true;

    // 処理時間計測
    QTimer*       m_processTimer  = nullptr;
    QElapsedTimer m_elapsedTimer;
    QLabel*       m_timeLabel     = nullptr;

    // 全体設定
    AppSettings   m_appSettings;
};
