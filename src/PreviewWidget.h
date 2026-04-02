#pragma once
#include <QWidget>
#include <QLabel>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QSlider>
#include <QPushButton>
#include <QGroupBox>
#include "FileItem.h"

// -------------------------------------------------------
// 1つのプレビューパネル (元 or 処理後)
// -------------------------------------------------------
class PreviewPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewPanel(const QString& title, QWidget* parent = nullptr);

    void loadFile(const QString& path, MediaType type);
    void showPlaceholder(const QString& msg = {});

private slots:
    void onPlayPause();
    void onPositionChanged(qint64 pos);
    void onSeek(int val);

private:
    void setupUi(const QString& title);
    void clearMedia();

    QLabel*       m_imageLabel   = nullptr;
    QVideoWidget* m_videoWidget  = nullptr;
    QMediaPlayer* m_player       = nullptr;
    QAudioOutput* m_audioOutput  = nullptr;

    QWidget*      m_controlPanel = nullptr;
    QPushButton*  m_playBtn      = nullptr;
    QSlider*      m_seekSlider   = nullptr;
    QLabel*       m_timeLabel    = nullptr;
    QLabel*       m_infoLabel    = nullptr;
    QLabel*       m_titleLabel   = nullptr;
};

// -------------------------------------------------------
// 元ファイル / 処理後を横並びで表示するウィジェット
// -------------------------------------------------------
class PreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void loadBefore(const FileItem& item);
    void loadAfter(const QString& outputPath, MediaType type);
    void clearAfter();
    void clearAll();

private:
    PreviewPanel* m_beforePanel = nullptr;
    PreviewPanel* m_afterPanel  = nullptr;
};
