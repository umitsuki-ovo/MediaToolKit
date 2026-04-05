#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QFormLayout>
#include "FFmpegProcessor.h"
#include "MediaTypes.h"
#include "SettingsDialog.h"

class ConvertWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConvertWidget(QWidget* parent = nullptr);

    void setSourceMediaType(MediaType type);
    void applyAppSettings(const AppSettings& s);
    ConvertOptions buildOptions() const;

private slots:
    void onConvertTargetChanged(int index);
    void onFormatChanged(int index);

private:
    void setupUi();
    void updateFormatCombo();
    void updateCodecRow();      // フォーマットに応じてコーデック行を切替

    MediaType     m_sourceType    = MediaType::Unknown;
    ConvertTarget m_currentTarget = ConvertTarget::VideoToVideo;

    QLabel*    m_typeLabel      = nullptr;
    QWidget*   m_targetRow      = nullptr;
    QComboBox* m_targetCombo    = nullptr;
    QComboBox* m_formatCombo    = nullptr;

    // 動画コーデック (mp4 のみ)
    QWidget*   m_videoCodecRow  = nullptr;
    QComboBox* m_videoCodecCombo= nullptr;

    // 音声コーデック (m4a のみ: AAC / ALAC)
    QWidget*   m_audioCodecRow  = nullptr;
    QComboBox* m_audioCodecCombo= nullptr;

    // フレーム指定 (動画→画像のみ)
    QWidget*   m_frameRow       = nullptr;
    QComboBox* m_frameModeCombo = nullptr;
    QSpinBox*  m_frameTimeSpin  = nullptr;
    QSpinBox*  m_frameNumSpin   = nullptr;

    QLineEdit*   m_outputDirEdit = nullptr;
    QPushButton* m_browseDirBtn  = nullptr;

    AppSettings  m_appSettings;
};
