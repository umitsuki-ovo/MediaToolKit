#pragma once
#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include "FFmpegProcessor.h"

class VideoSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoSettingsWidget(QWidget* parent = nullptr);

    // 圧縮設定を CompressOptions に反映
    void applyToCompressOptions(CompressOptions& opts) const;

    // 変換設定を ConvertOptions に反映
    void applyToConvertOptions(ConvertOptions& opts) const;

    // 変換モード切り替え (動画→動画 / 動画→音声 / 動画→画像)
    void setConvertTarget(ConvertTarget target);

private:
    void setupUi();

    // 解像度
    QComboBox* m_resCombo   = nullptr;
    QLabel*    m_resLabel   = nullptr;
    QSpinBox*  m_customW    = nullptr;
    QSpinBox*  m_customH    = nullptr;

    // FPS
    QSlider*   m_fpsSlider  = nullptr;
    QLabel*    m_fpsLabel   = nullptr;

    // コーデック
    QComboBox* m_codecCombo = nullptr;

    // フレーム抽出時刻 (変換→画像)
    QWidget*   m_frameWidget = nullptr;
    QComboBox* m_frameMode   = nullptr;  // 時間 or フレーム番号
    QSpinBox*  m_frameTime   = nullptr;  // 秒
    QSpinBox*  m_frameNum    = nullptr;

    ConvertTarget m_convertTarget = ConvertTarget::VideoToVideo;
};
