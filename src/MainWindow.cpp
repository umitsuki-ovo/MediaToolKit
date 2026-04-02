#include "MainWindow.h"
#include "SettingsDialog.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QWidgetAction>
#include <QStatusBar>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QLabel>
#include <QFrame>

// -------------------------------------------------------
// コンストラクタ
// -------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("MediaToolKit");
    setMinimumSize(980, 600);
    resize(1280, 760);

    setupMenuBar();  // ← setupUi より先に呼ぶ
    setupUi();
    setupToolBar();

    // ダークスタイル
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #1e1e1e;
            color: #ddd;
            font-family: "Yu Gothic UI", "Meiryo", sans-serif;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #3a3a3a;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 4px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 6px;
            color: #999;
            font-weight: bold;
        }
        QTabWidget::pane { border: 1px solid #3a3a3a; border-radius: 4px; }
        QTabBar::tab {
            background: #2a2a2a; border: 1px solid #3a3a3a;
            padding: 6px 16px; border-bottom: none;
            border-radius: 4px 4px 0 0; color: #bbb;
        }
        QTabBar::tab:selected { background: #3a3a3a; color: #fff; }
        QPushButton {
            background-color: #2d5a8e; color: #fff;
            border: none; border-radius: 5px;
            padding: 6px 16px; font-weight: bold;
        }
        QPushButton:hover   { background-color: #3a72b0; }
        QPushButton:pressed { background-color: #1f4070; }
        QPushButton:disabled{ background-color: #333; color: #666; }
        QPushButton#startBtn {
            background-color: #2d8e4a;
            font-size: 14px; padding: 8px 24px;
        }
        QPushButton#startBtn:hover    { background-color: #3ab060; }
        QPushButton#startBtn:disabled { background-color: #2a3a2a; color: #556; }
        QPushButton#stopBtn  { background-color: #8e2d2d; }
        QPushButton#stopBtn:hover { background-color: #b03a3a; }
        QPushButton#fileBtn {
            background-color: #2a2a2a; color: #ccc;
            border: 1px solid #3a3a3a; border-radius: 4px;
            padding: 4px 12px; font-weight: normal;
        }
        QPushButton#fileBtn:hover { background-color: #353535; }
        /* ツールバーの圧縮/変換トグルボタン */
        QPushButton#modeBtn {
            background-color: transparent; color: #aaa;
            border: 1px solid #444; border-radius: 4px;
            padding: 4px 18px; font-weight: bold; font-size: 13px;
            min-width: 60px;
        }
        QPushButton#modeBtn:checked {
            background-color: #2d5a8e; border-color: #2d5a8e; color: #fff;
        }
        QPushButton#modeBtn:hover:!checked { background-color: #2e2e2e; }
        QListWidget {
            background-color: #252525; border: 1px solid #3a3a3a;
            border-radius: 4px; alternate-background-color: #2a2a2a;
        }
        QListWidget::item:selected { background-color: #2d5a8e; color: #fff; }
        QScrollArea { border: none; background: transparent; }
        QScrollBar:vertical {
            background: #252525; width: 8px; border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #555; border-radius: 4px; min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QLineEdit, QSpinBox {
            background: #2a2a2a; border: 1px solid #3a3a3a;
            border-radius: 4px; padding: 3px 6px; color: #ddd;
        }
        /* SpinBox の増減ボタンを非表示 */
        QSpinBox::up-button, QSpinBox::down-button {
            width: 0; height: 0; border: none; background: transparent;
        }
        QSpinBox::up-arrow, QSpinBox::down-arrow {
            width: 0; height: 0; border: none; image: none;
        }
        QComboBox {
            background: #2a2a2a; border: 1px solid #3a3a3a;
            border-radius: 4px; padding: 3px 24px 3px 8px;
            color: #ddd; min-width: 80px;
        }
        /* ▼ 矢印エリア */
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: center right;
            width: 22px; border: none;
            background: transparent;
        }
        /* ▼ を SVG で描画 */
        QComboBox::down-arrow {
            image: url(:/icons/arrow_down.svg);
            width: 12px; height: 12px;
        }
        QComboBox QAbstractItemView {
            background: #2a2a2a; border: 1px solid #4a4a4a;
            color: #ddd; outline: none;
            selection-background-color: #2d5a8e;
            selection-color: #fff;
        }
        QComboBox QAbstractItemView::item {
            padding: 4px 8px; color: #ddd;
            background: #2a2a2a;
        }
        QComboBox QAbstractItemView::item:alternate {
            background: #2e2e2e;
        }
        QSlider::groove:horizontal {
            height: 4px; background: #3a3a3a; border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #2d5a8e; width: 14px; height: 14px;
            margin: -5px 0; border-radius: 7px;
        }
        QSlider::sub-page:horizontal { background: #2d5a8e; border-radius: 2px; }
        QProgressBar {
            background: #2a2a2a; border: 1px solid #3a3a3a;
            border-radius: 4px; text-align: center; color: #fff;
        }
        QProgressBar::chunk { background: #2d8e4a; border-radius: 3px; }
        QToolBar {
            background: #252525; border-bottom: 1px solid #3a3a3a;
            spacing: 6px; padding: 4px 8px;
        }
        QToolButton {
            background: transparent; border: none; border-radius: 4px;
            padding: 4px 10px; color: #ccc;
        }
        QToolButton:hover { background: #3a3a3a; }
        QMenuBar { background: #252525; border-bottom: 1px solid #333; color: #ccc; }
        QMenuBar::item:selected { background: #3a3a3a; }
        QMenu { background: #252525; border: 1px solid #3a3a3a; color: #ddd; }
        QMenu::item:selected { background: #2d5a8e; }
        QStatusBar { background: #252525; border-top: 1px solid #333; color: #888; }
        QSplitter::handle { background: #2e2e2e; }
        QCheckBox { color: #ddd; spacing: 6px; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border: 1.5px solid #555; border-radius: 3px;
            background: #2a2a2a;
        }
        QCheckBox::indicator:hover {
            border-color: #2d5a8e;
        }
        QCheckBox::indicator:checked {
            background: #2d5a8e; border-color: #2d5a8e;
            image: url(:/icons/check.svg);
        }
        QCheckBox::indicator:checked:hover {
            background: #3a72b0; border-color: #3a72b0;
            image: url(:/icons/check.svg);
        }
        QRadioButton { color: #ddd; spacing: 6px; }
        QRadioButton::indicator {
            width: 16px; height: 16px;
            border: 1.5px solid #555; border-radius: 8px;
            background: #2a2a2a;
        }
        QRadioButton::indicator:hover { border-color: #2d5a8e; }
        QRadioButton::indicator:checked {
            background: #2d5a8e; border-color: #2d5a8e;
            /* 中央に白丸 */
            image: none;
        }
        QRadioButton::indicator:checked {
            background: radial-gradient(circle, white 35%, #2d5a8e 35%);
            border-color: #2d5a8e;
        }
        QLabel { color: #ddd; }
    )");

    // 設定を読み込んで適用
    m_appSettings = SettingsDialog::loadSettings();
    applySettingsToWidgets();

    // 初期状態
    updateStartButton();
    applyMode(true);  // 圧縮モードで起動
}

MainWindow::~MainWindow()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestStop();
        m_worker->wait(3000);
    }
}

// -------------------------------------------------------
// UI 構築
// -------------------------------------------------------
void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLay = new QVBoxLayout(central);
    mainLay->setContentsMargins(0,0,0,0);
    mainLay->setSpacing(0);

    // ============================================================
    // メインスプリッタ (左: ファイル一覧  右: 設定+プレビュー)
    // ============================================================
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(5);

    // --------------------------------------------------------
    // 左ペイン: ファイル一覧 + ファイル操作ボタン
    // --------------------------------------------------------
    auto* leftPanel = new QWidget(this);
    auto* leftLay   = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(8,8,8,8);
    leftLay->setSpacing(5);

    // ヘッダ
    auto* hdrRow = new QHBoxLayout;
    auto* hdrLbl = new QLabel("ファイル一覧", this);
    hdrLbl->setStyleSheet("font-weight:bold; font-size:14px; color:#fff;");
    m_statusLabel = new QLabel("0 ファイル", this);
    m_statusLabel->setStyleSheet("color:#888;");
    hdrRow->addWidget(hdrLbl);
    hdrRow->addStretch();
    hdrRow->addWidget(m_statusLabel);
    leftLay->addLayout(hdrRow);

    // ファイルリスト
    m_fileList = new FileListWidget(this);
    leftLay->addWidget(m_fileList, 1);

    // D&D ヒント (ファイルがないとき表示)
    auto* dropHint = new QLabel("ここにファイルをドロップ", m_fileList);
    dropHint->setAlignment(Qt::AlignCenter);
    dropHint->setStyleSheet("color:#444; font-size:13px;");
    dropHint->setAttribute(Qt::WA_TransparentForMouseEvents);

    // ファイル操作ボタン
    auto* fileBtnRow = new QHBoxLayout;
    fileBtnRow->setSpacing(4);
    auto mkFileBtn = [this](const QString& text) {
        auto* b = new QPushButton(text, this);
        b->setObjectName("fileBtn");
        b->setFixedHeight(28);
        return b;
    };
    auto* addBtn    = mkFileBtn("追加");
    auto* removeBtn = mkFileBtn("削除");
    auto* clearBtn  = mkFileBtn("クリア");
    fileBtnRow->addWidget(addBtn);
    fileBtnRow->addWidget(removeBtn);
    fileBtnRow->addWidget(clearBtn);
    leftLay->addLayout(fileBtnRow);

    connect(addBtn,    &QPushButton::clicked, this, &MainWindow::onAddFiles);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveFiles);
    connect(clearBtn,  &QPushButton::clicked, this, &MainWindow::onClearFiles);

    connect(m_fileList, &FileListWidget::filesChanged, this, [this, dropHint]{
        dropHint->setVisible(m_fileList->fileItems().isEmpty());
        onFilesChanged();
    });
    connect(m_fileList, &FileListWidget::fileSelected,
            this, &MainWindow::onFileSelected);

    m_mainSplitter->addWidget(leftPanel);

    // --------------------------------------------------------
    // 右ペイン: 設定スタック + プレビュー (縦スプリッタ)
    // --------------------------------------------------------
    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_rightSplitter->setHandleWidth(5);

    m_settingsStack  = new QStackedWidget(this);
    m_compressWidget = new CompressWidget(this);
    m_convertWidget  = new ConvertWidget(this);
    m_settingsStack->addWidget(m_compressWidget);   // index 0
    m_settingsStack->addWidget(m_convertWidget);    // index 1
    m_rightSplitter->addWidget(m_settingsStack);

    // プレビュー (圧縮モードのみ表示)
    m_previewBox = new QGroupBox("プレビュー (元ファイル / 処理後)", this);
    auto* pvLay  = new QVBoxLayout(m_previewBox);
    pvLay->setContentsMargins(4,6,4,4);
    m_previewWidget = new PreviewWidget(this);
    pvLay->addWidget(m_previewWidget);
    m_rightSplitter->addWidget(m_previewBox);

    m_rightSplitter->setStretchFactor(0, 3);
    m_rightSplitter->setStretchFactor(1, 2);

    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setStretchFactor(0, 2);
    m_mainSplitter->setStretchFactor(1, 3);

    mainLay->addWidget(m_mainSplitter, 1);

    // ============================================================
    // 下部バー: 進捗 + 処理開始/停止
    // ============================================================
    auto* bottomBar = new QWidget(this);
    bottomBar->setStyleSheet("background:#252525; border-top:1px solid #333;");
    auto* btmLay = new QHBoxLayout(bottomBar);
    btmLay->setContentsMargins(12,8,12,8);
    btmLay->setSpacing(8);

    m_totalProgress = new QProgressBar(this);
    m_totalProgress->setRange(0, 1000);
    m_totalProgress->setValue(0);
    m_totalProgress->setFormat("%p%");
    m_totalProgress->setFixedHeight(22);
    m_totalProgress->setMinimumWidth(200);

    // 経過時間ラベル
    m_timeLabel = new QLabel("--:--", this);
    m_timeLabel->setStyleSheet(
        "color:#888; font-size:12px; font-family:monospace; min-width:52px;");
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_startBtn = new QPushButton("処理開始", this);
    m_startBtn->setObjectName("startBtn");
    m_startBtn->setFixedHeight(40);
    m_startBtn->setMinimumWidth(140);
    m_startBtn->setEnabled(false);

    m_stopBtn = new QPushButton("停止", this);
    m_stopBtn->setObjectName("stopBtn");
    m_stopBtn->setFixedHeight(40);
    m_stopBtn->setMinimumWidth(90);
    m_stopBtn->setEnabled(false);

    btmLay->addWidget(m_totalProgress);
    btmLay->addWidget(m_timeLabel);
    btmLay->addStretch();
    btmLay->addWidget(m_stopBtn);
    btmLay->addWidget(m_startBtn);
    mainLay->addWidget(bottomBar);

    // 処理時間タイマー (1秒ごと更新)
    m_processTimer = new QTimer(this);
    m_processTimer->setInterval(1000);
    connect(m_processTimer, &QTimer::timeout, this, [this]{
        qint64 sec = m_elapsedTimer.elapsed() / 1000;
        int m = (int)(sec / 60), s = (int)(sec % 60);
        m_timeLabel->setText(QString("%1:%2")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
        m_timeLabel->setStyleSheet(
            "color:#6ab0f5; font-size:12px; font-family:monospace; min-width:52px;");
    });

    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartProcess);
    connect(m_stopBtn,  &QPushButton::clicked, this, &MainWindow::onStopProcess);
}

// -------------------------------------------------------
// メニューバー: 設定・出力フォルダ・ヘルプ
// -------------------------------------------------------
void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu("ファイル(&F)");
    auto* quitAct  = fileMenu->addAction("終了(&Q)");
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

    auto* toolMenu   = menuBar()->addMenu("ツール(&T)");
    auto* settAct    = toolMenu->addAction("設定(&S)...");
    settAct->setShortcut(Qt::CTRL | Qt::Key_Comma);
    connect(settAct, &QAction::triggered, this, &MainWindow::onOpenSettings);
    toolMenu->addSeparator();
    auto* outAct     = toolMenu->addAction("出力フォルダを開く(&O)");
    connect(outAct, &QAction::triggered, this, &MainWindow::onOpenOutputFolder);

    auto* helpMenu  = menuBar()->addMenu("ヘルプ(&H)");
    auto* aboutAct  = helpMenu->addAction("バージョン情報(&A)");
    connect(aboutAct, &QAction::triggered, this, [this]{
        QMessageBox::about(this, "MediaToolKit",
            "<b>MediaToolKit v1.00</b><br><br>"
            "画像・動画・音声の圧縮・変換ソフト<br><br>"
            "使用ライブラリ: Qt 6.7 / FFmpeg 7.0");
    });
}

// -------------------------------------------------------
// ツールバー: 圧縮/変換 トグルボタンのみ
// -------------------------------------------------------
void MainWindow::setupToolBar()
{
    auto* tb = addToolBar("モード選択");
    tb->setMovable(false);
    tb->setFloatable(false);

    // --- 圧縮ボタン ---
    m_tbCompressBtn = new QPushButton("圧縮", tb);
    m_tbCompressBtn->setObjectName("modeBtn");
    m_tbCompressBtn->setCheckable(true);
    m_tbCompressBtn->setChecked(true);
    m_tbCompressBtn->setFixedHeight(30);

    auto* compAct = new QWidgetAction(tb);
    compAct->setDefaultWidget(m_tbCompressBtn);
    tb->addAction(compAct);

    // --- 変換ボタン ---
    m_tbConvertBtn = new QPushButton("変換", tb);
    m_tbConvertBtn->setObjectName("modeBtn");
    m_tbConvertBtn->setCheckable(true);
    m_tbConvertBtn->setChecked(false);
    m_tbConvertBtn->setFixedHeight(30);

    auto* convAct = new QWidgetAction(tb);
    convAct->setDefaultWidget(m_tbConvertBtn);
    tb->addAction(convAct);

    // 排他トグル: 直接 clicked を接続
    connect(m_tbCompressBtn, &QPushButton::clicked, this, &MainWindow::onSwitchCompress);
    connect(m_tbConvertBtn,  &QPushButton::clicked, this, &MainWindow::onSwitchConvert);
}

// -------------------------------------------------------
// モード切り替え
// -------------------------------------------------------
void MainWindow::onSwitchCompress()
{
    m_tbCompressBtn->setChecked(true);
    m_tbConvertBtn->setChecked(false);
    applyMode(true);
}

void MainWindow::onSwitchConvert()
{
    m_tbCompressBtn->setChecked(false);
    m_tbConvertBtn->setChecked(true);
    applyMode(false);
}

void MainWindow::applyMode(bool compress)
{
    m_compressMode = compress;
    m_settingsStack->setCurrentIndex(compress ? 0 : 1);

    // プレビューは圧縮時のみ
    m_previewBox->setVisible(compress);

    // ファイルリストのサイズ列は圧縮時のみ
    m_fileList->setShowSizeInfo(compress);

    // 選択中ファイルに合わせて設定パネルを更新
    int row = m_fileList->currentRow();
    const auto& items = m_fileList->fileItems();
    if (row >= 0 && row < items.size()) {
        if (compress)
            m_compressWidget->setMediaType(items[row].mediaType);
        else
            m_convertWidget->setSourceMediaType(items[row].mediaType);
    }
}

// -------------------------------------------------------
// ファイル操作
// -------------------------------------------------------
void MainWindow::onAddFiles()
{
    QStringList exts;
    for (const auto& e : IMAGE_EXTENSIONS) exts << "*." + e;
    for (const auto& e : VIDEO_EXTENSIONS) exts << "*." + e;
    for (const auto& e : AUDIO_EXTENSIONS) exts << "*." + e;

    QStringList paths = QFileDialog::getOpenFileNames(
        this, "ファイルを追加", {},
        "メディアファイル (" + exts.join(" ") + ");;すべてのファイル (*.*)");
    if (!paths.isEmpty())
        m_fileList->addFiles(paths);
}

void MainWindow::onRemoveFiles() { m_fileList->removeSelected(); }

void MainWindow::onClearFiles()
{
    if (m_fileList->fileItems().isEmpty()) return;
    if (QMessageBox::question(this, "確認", "リストをクリアしますか?",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_fileList->clearAll();
        m_previewWidget->clearAll();
    }
}

// -------------------------------------------------------
// ファイル選択
// -------------------------------------------------------
void MainWindow::onFileSelected(const FileItem& item)
{
    if (m_compressMode) {
        m_compressWidget->setMediaType(item.mediaType);
        m_previewWidget->loadBefore(item);
        if (item.status == FileStatus::Done && !item.outputPath.isEmpty())
            m_previewWidget->loadAfter(item.outputPath, item.mediaType);
        else
            m_previewWidget->clearAfter();
    } else {
        m_convertWidget->setSourceMediaType(item.mediaType);
    }
}

// -------------------------------------------------------
// ファイルリスト変更
// -------------------------------------------------------
void MainWindow::onFilesChanged()
{
    int n = m_fileList->fileItems().size();
    m_statusLabel->setText(QString("%1 ファイル").arg(n));
    m_totalProgress->setRange(0, std::max(n, 1) * 1000);
    m_totalProgress->setValue(0);
    updateStartButton();
}

void MainWindow::updateStartButton()
{
    bool hasFiles = !m_fileList->fileItems().isEmpty();
    m_startBtn->setEnabled(hasFiles);
}

// -------------------------------------------------------
// 処理開始
// -------------------------------------------------------
void MainWindow::onStartProcess()
{
    const auto& items = m_fileList->fileItems();
    if (items.isEmpty()) return;

    // 変換モードのとき: フォーマットが設定されているか確認
    if (!m_compressMode) {
        ConvertOptions testOpts = m_convertWidget->buildOptions();
        if (testOpts.format.isEmpty()) {
            QMessageBox::warning(this, "設定エラー",
                "出力フォーマットが指定されていません。\n"
                "ファイルを選択して変換設定を確認してください。");
            return;
        }
    }

    m_fileList->resetAllStatus();
    if (m_compressMode) m_previewWidget->clearAfter();
    m_doneFiles  = 0;
    m_totalFiles = items.size();
    // ★ 0.1% 単位で滑らかに表示 (range: 0..totalFiles*1000)
    m_totalProgress->setRange(0, m_totalFiles * 1000);
    m_totalProgress->setFormat("%p%");
    m_totalProgress->setValue(0);

    // WorkerThread 生成
    m_worker = new BatchWorker(this);
    m_worker->setFiles(items);
    m_worker->setCompressMode(m_compressMode);

    if (m_compressMode) {
        CompressOptions opts = m_compressWidget->buildOptions();
        m_lastOutputDir      = opts.outputDir;
        m_worker->setCompressOptions(opts);
    } else {
        ConvertOptions opts = m_convertWidget->buildOptions();
        m_lastOutputDir     = opts.outputDir;
        m_worker->setConvertOptions(opts);
    }

    // シグナル接続
    connect(m_worker, &BatchWorker::fileStarted,
            this, &MainWindow::onFileStarted);
    connect(m_worker, &BatchWorker::fileProgress,
            this, &MainWindow::onFileProgress);
    connect(m_worker, &BatchWorker::fileFinished,
            this, &MainWindow::onFileFinished);
    connect(m_worker, &BatchWorker::allFinished,
            this, &MainWindow::onAllFinished);
    connect(m_worker, &QThread::finished,
            m_worker, &QObject::deleteLater);

    setProcessing(true);
    // 処理時間タイマー開始
    m_elapsedTimer.start();
    m_timeLabel->setText("00:00");
    m_timeLabel->setStyleSheet(
        "color:#6ab0f5; font-size:12px; font-family:monospace; min-width:52px;");
    m_processTimer->start();
    m_worker->start();
    statusBar()->showMessage(
        QString("処理中... 0 / %1").arg(m_totalFiles));
}

void MainWindow::onStopProcess()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestStop();
        statusBar()->showMessage("停止中...");
        // UI のブロックを避けつつ終了を待つ (最大3秒)
        m_worker->wait(3000);
    }
}

// -------------------------------------------------------
// BatchWorker コールバック
// -------------------------------------------------------
void MainWindow::onFileStarted(int index)
{
    m_fileList->updateItemStatus(index, FileStatus::Processing);
    const auto& items = m_fileList->fileItems();
    if (index < items.size())
        statusBar()->showMessage(
            QString("処理中: %1  (%2 / %3)")
            .arg(items[index].name)
            .arg(index + 1)
            .arg(m_totalFiles));
}

void MainWindow::onFileProgress(int /*index*/, float progress)
{
    // ★ 処理中ファイルの進捗を反映
    int val = (int)((m_doneFiles + (double)progress) * 1000.0);
    m_totalProgress->setValue(val);
}

void MainWindow::onFileFinished(int index, bool ok, const QString& error,
                                 qint64 outputBytes, const QString& outputPath)
{
    // "キャンセルされました" はエラーではなく Skipped 扱い
    bool cancelled = error.contains("キャンセル");
    FileStatus st = ok ? FileStatus::Done
                       : (cancelled ? FileStatus::Skipped : FileStatus::Error);

    m_fileList->updateItemStatus(index, st, error, outputBytes, outputPath);

    if (ok && m_compressMode && index == m_fileList->currentRow()
        && !outputPath.isEmpty())
    {
        const auto& items = m_fileList->fileItems();
        MediaType mt = (index < items.size())
                       ? items[index].mediaType : MediaType::Unknown;
        m_previewWidget->loadAfter(outputPath, mt);
    }

    m_doneFiles++;
    m_totalProgress->setValue(m_doneFiles * 1000);
}

void MainWindow::onAllFinished()
{
    m_processTimer->stop();
    qint64 totalSec = m_elapsedTimer.elapsed() / 1000;
    int mm = (int)(totalSec / 60), ss = (int)(totalSec % 60);
    QString elapsed = QString("%1:%2").arg(mm,2,10,QChar('0')).arg(ss,2,10,QChar('0'));
    m_timeLabel->setStyleSheet(
        "color:#6abf6a; font-size:12px; font-family:monospace; min-width:52px;");

    setProcessing(false);

    int errors   = 0;
    int skipped  = 0;
    int done     = 0;
    for (const auto& it : m_fileList->fileItems()) {
        if (it.status == FileStatus::Error)   errors++;
        if (it.status == FileStatus::Skipped) skipped++;
        if (it.status == FileStatus::Done)    done++;
    }

    if (skipped > 0 && done == 0 && errors == 0) {
        // 全キャンセル
        statusBar()->showMessage(QString("停止: %1 ファイルをキャンセルしました [%2]")
                                 .arg(skipped).arg(elapsed));
        m_timeLabel->setStyleSheet(
            "color:#888; font-size:12px; font-family:monospace; min-width:52px;");
    } else if (errors == 0) {
        statusBar()->showMessage(
            QString("完了: %1 ファイル処理しました  [%2]")
            .arg(m_totalFiles).arg(elapsed));
        QMessageBox::information(this, "完了",
            QString("%1 ファイルの処理が完了しました。\n処理時間: %2")
            .arg(done).arg(elapsed));
    } else {
        statusBar()->showMessage(
            QString("完了 (エラーあり): %1 成功 / %2 エラー  [%3]")
            .arg(done).arg(errors).arg(elapsed));
        QMessageBox::warning(this, "完了 (エラーあり)",
            QString("%1 ファイル中 %2 件でエラーが発生しました。\n"
                    "処理時間: %3\n"
                    "リスト内の赤いアイテムを確認してください。")
            .arg(m_totalFiles).arg(errors).arg(elapsed));
    }
}

// -------------------------------------------------------
// UI 状態管理
// -------------------------------------------------------
void MainWindow::setProcessing(bool processing)
{
    bool hasFiles = !m_fileList->fileItems().isEmpty();
    m_startBtn->setEnabled(!processing && hasFiles);
    m_stopBtn->setEnabled(processing);
    m_tbCompressBtn->setEnabled(!processing);
    m_tbConvertBtn->setEnabled(!processing);
    m_fileList->setEnabled(!processing);
    m_settingsStack->setEnabled(!processing);
}

// -------------------------------------------------------
// 設定
// -------------------------------------------------------
void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(m_appSettings, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_appSettings = dlg.getSettings();
        SettingsDialog::saveSettings(m_appSettings);   // ★ 永続化
        applySettingsToWidgets();
    }
}

void MainWindow::applySettingsToWidgets()
{
    m_compressWidget->applyAppSettings(m_appSettings);
    m_convertWidget->applyAppSettings(m_appSettings);
}

// -------------------------------------------------------
// 出力フォルダを開く
// -------------------------------------------------------
void MainWindow::onOpenOutputFolder()
{
    QString dir = m_lastOutputDir;
    if (dir.isEmpty() && !m_fileList->fileItems().isEmpty())
        dir = QFileInfo(m_fileList->fileItems().first().path).absolutePath();
    if (dir.isEmpty() || !QDir(dir).exists()) {
        QMessageBox::information(this, "情報",
            "出力フォルダが見つかりません。");
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}
