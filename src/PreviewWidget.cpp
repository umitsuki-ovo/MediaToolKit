#include "PreviewWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileInfo>
#include <QPixmap>
#include <QStackedWidget>
#include <QFrame>

// ============================================================
// PreviewPanel
// ============================================================
PreviewPanel::PreviewPanel(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    setupUi(title);
}

void PreviewPanel::setupUi(const QString& title)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4,4,4,4);
    root->setSpacing(4);

    // タイトル
    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        "font-weight:bold; font-size:12px; color:#aaa;"
        "background:#252525; border-radius:3px; padding:3px 0;");
    root->addWidget(m_titleLabel);

    // 画像表示ラベル
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(200, 140);
    m_imageLabel->setStyleSheet("background:#1a1a1a; border-radius:4px;");
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_imageLabel, 1);

    // 動画表示
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumSize(200, 140);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->hide();
    root->addWidget(m_videoWidget, 1);

    // メディアプレイヤー
    m_player      = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(m_videoWidget);

    // コントロールパネル
    m_controlPanel = new QWidget(this);
    auto* ctrlLay  = new QHBoxLayout(m_controlPanel);
    ctrlLay->setContentsMargins(0,0,0,0);
    ctrlLay->setSpacing(4);

    m_playBtn    = new QPushButton("▶", this);
    m_playBtn->setFixedSize(30, 24);
    m_playBtn->setStyleSheet(
        "QPushButton{background:#2d5a8e;border-radius:3px;font-size:10px;padding:0;}"
        "QPushButton:hover{background:#3a72b0;}");

    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 1000);
    m_seekSlider->setFixedHeight(16);

    m_timeLabel  = new QLabel("0:00", this);
    m_timeLabel->setFixedWidth(40);
    m_timeLabel->setStyleSheet("color:#888; font-size:10px;");
    m_timeLabel->setAlignment(Qt::AlignCenter);

    ctrlLay->addWidget(m_playBtn);
    ctrlLay->addWidget(m_seekSlider);
    ctrlLay->addWidget(m_timeLabel);
    m_controlPanel->hide();
    root->addWidget(m_controlPanel);

    // 情報ラベル
    m_infoLabel = new QLabel(this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet("color:#666; font-size:10px;");
    root->addWidget(m_infoLabel);

    // シグナル
    connect(m_playBtn,    &QPushButton::clicked,         this, &PreviewPanel::onPlayPause);
    connect(m_seekSlider, &QSlider::sliderMoved,         this, &PreviewPanel::onSeek);
    connect(m_player, &QMediaPlayer::positionChanged,    this, &PreviewPanel::onPositionChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState s) {
        m_playBtn->setText(s == QMediaPlayer::PlayingState ? "⏸" : "▶");
    });

    // 初期プレースホルダー
    showPlaceholder();
}

void PreviewPanel::showPlaceholder(const QString& msg)
{
    clearMedia();
    m_imageLabel->show();
    m_imageLabel->setPixmap({});
    m_imageLabel->setText(msg.isEmpty() ? "ファイルを選択してください" : msg);
    m_infoLabel->clear();
    m_controlPanel->hide();
}

void PreviewPanel::clearMedia()
{
    m_player->stop();
    m_player->setSource({});
    m_imageLabel->hide();
    m_videoWidget->hide();
    m_controlPanel->hide();
    m_infoLabel->clear();
}

void PreviewPanel::loadFile(const QString& path, MediaType type)
{
    clearMedia();

    if (path.isEmpty() || !QFileInfo::exists(path)) {
        showPlaceholder("ファイルが見つかりません");
        return;
    }

    switch (type) {
        case MediaType::Image: {
            QPixmap px(path);
            if (px.isNull()) { showPlaceholder("読み込みエラー"); return; }
            m_imageLabel->show();
            // ラベルサイズに合わせてスケール
            QSize maxSz = m_imageLabel->size().isEmpty()
                          ? QSize(320,200) : m_imageLabel->size();
            m_imageLabel->setPixmap(
                px.scaled(maxSz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_infoLabel->setText(QString("%1 x %2 px  |  %3")
                .arg(px.width()).arg(px.height())
                .arg(QFileInfo(path).fileName()));
            break;
        }
        case MediaType::Video: {
            m_videoWidget->show();
            m_controlPanel->show();
            m_player->setSource(QUrl::fromLocalFile(path));
            m_player->play();
            m_infoLabel->setText(QFileInfo(path).fileName());
            break;
        }
        case MediaType::Audio: {
            m_imageLabel->show();
            m_imageLabel->setPixmap({});
            m_imageLabel->setText("音声ファイル\n" + QFileInfo(path).fileName());
            m_controlPanel->show();
            m_player->setSource(QUrl::fromLocalFile(path));
            m_infoLabel->setText(QFileInfo(path).fileName());
            break;
        }
        default:
            showPlaceholder("非対応フォーマット");
    }
}

void PreviewPanel::onPlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        m_player->play();
}

void PreviewPanel::onPositionChanged(qint64 pos)
{
    if (m_player->duration() > 0)
        m_seekSlider->setValue((int)(pos * 1000 / m_player->duration()));

    int s = (int)(pos / 1000);
    m_timeLabel->setText(QString("%1:%2")
        .arg(s / 60).arg(s % 60, 2, 10, QChar('0')));
}

void PreviewPanel::onSeek(int val)
{
    if (m_player->duration() > 0)
        m_player->setPosition(val * m_player->duration() / 1000);
}

// ============================================================
// PreviewWidget (元 + 処理後 横並び)
// ============================================================
PreviewWidget::PreviewWidget(QWidget* parent) : QWidget(parent)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(4);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);

    m_beforePanel = new PreviewPanel("元ファイル", this);
    m_afterPanel  = new PreviewPanel("処理後", this);

    splitter->addWidget(m_beforePanel);
    splitter->addWidget(m_afterPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    root->addWidget(splitter);
}

void PreviewWidget::loadBefore(const FileItem& item)
{
    m_beforePanel->loadFile(item.path, item.mediaType);
}

void PreviewWidget::loadAfter(const QString& outputPath, MediaType type)
{
    if (outputPath.isEmpty() || !QFileInfo::exists(outputPath)) {
        m_afterPanel->showPlaceholder("処理後のファイルはここに表示されます");
        return;
    }
    m_afterPanel->loadFile(outputPath, type);
}

void PreviewWidget::clearAfter()
{
    m_afterPanel->showPlaceholder("処理後のファイルはここに表示されます");
}

void PreviewWidget::clearAll()
{
    m_beforePanel->showPlaceholder();
    m_afterPanel->showPlaceholder("処理後のファイルはここに表示されます");
}
