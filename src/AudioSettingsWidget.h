#pragma once
#include <QWidget>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QRadioButton>
#include "FFmpegProcessor.h"

class AudioSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioSettingsWidget(QWidget* parent = nullptr);
    void applyToCompressOptions(CompressOptions& opts) const;

private:
    void setupUi();

    QSlider*      m_bitrateSlider  = nullptr;
    QLabel*       m_bitrateLabel   = nullptr;
    QComboBox*    m_sampleRateCombo= nullptr;
    QComboBox*    m_bitDepthCombo  = nullptr;
    QRadioButton* m_qualityFirst   = nullptr;
    QRadioButton* m_sizeFirst      = nullptr;
};
