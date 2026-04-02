#pragma once
#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "FFmpegProcessor.h"
#include "MediaTypes.h"
#include "SettingsDialog.h"
#include "VideoSettingsWidget.h"
#include "AudioSettingsWidget.h"
#include "ImageSettingsWidget.h"

class CompressWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CompressWidget(QWidget* parent = nullptr);

    // 選択ファイルのメディア種別に応じて設定パネルを自動切替
    void setMediaType(MediaType type);
    void applyAppSettings(const AppSettings& s);
    CompressOptions buildOptions() const;

private:
    void setupUi();

    MediaType            m_currentType   = MediaType::Unknown;

    QLabel*              m_typeLabel     = nullptr; // 検出された種別表示
    QStackedWidget*      m_stack         = nullptr;
    VideoSettingsWidget* m_videoSettings = nullptr;
    AudioSettingsWidget* m_audioSettings = nullptr;
    ImageSettingsWidget* m_imageSettings = nullptr;

    QLineEdit*   m_outputDirEdit = nullptr;
    QPushButton* m_browseDirBtn  = nullptr;

    AppSettings  m_appSettings;
};
