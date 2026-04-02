#pragma once
#include <QWidget>
#include <QSlider>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include "FFmpegProcessor.h"

class ImageSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImageSettingsWidget(QWidget* parent = nullptr);
    void applyToCompressOptions(CompressOptions& opts) const;

private:
    void setupUi();

    QCheckBox* m_losslessCheck   = nullptr;
    QSlider*   m_qualitySlider   = nullptr;
    QLabel*    m_qualityLabel    = nullptr;   // 数値 (85 など)
    QLabel*    m_qualLevelLabel  = nullptr;   // 低品質/中品質/高品質
    QCheckBox* m_targetSizeCheck = nullptr;
    QSpinBox*  m_targetSizeBox   = nullptr;
    QComboBox* m_resCombo        = nullptr;
    QSpinBox*  m_customW         = nullptr;
    QSpinBox*  m_customH         = nullptr;
};
