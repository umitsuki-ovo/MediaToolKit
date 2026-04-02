#include "AudioSettingsWidget.h"
#include "MediaTypes.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>

AudioSettingsWidget::AudioSettingsWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

void AudioSettingsWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);

    // ---- 品質モード ----
    auto* modeGroup  = new QGroupBox("最適化モード", this);
    auto* modeLayout = new QVBoxLayout(modeGroup);
    m_qualityFirst = new QRadioButton("品質優先", this);
    m_sizeFirst    = new QRadioButton("サイズ優先", this);
    m_qualityFirst->setChecked(true);
    modeLayout->addWidget(m_qualityFirst);
    modeLayout->addWidget(m_sizeFirst);
    root->addWidget(modeGroup);

    // ---- ビットレート ----
    auto* brGroup  = new QGroupBox("ビットレート", this);
    auto* brLayout = new QVBoxLayout(brGroup);
    auto* brCheck  = new QCheckBox("変更しない", this);
    brCheck->setChecked(true);

    m_bitrateSlider = new QSlider(Qt::Horizontal, this);
    m_bitrateSlider->setRange(0, AUDIO_BITRATE_COUNT - 1);
    m_bitrateSlider->setValue(2); // 192 kbps デフォルト
    m_bitrateSlider->setTickPosition(QSlider::TicksBelow);
    m_bitrateSlider->setEnabled(false);

    m_bitrateLabel = new QLabel(AUDIO_BITRATE_LABELS[2], this);
    m_bitrateLabel->setAlignment(Qt::AlignCenter);

    connect(brCheck, &QCheckBox::toggled, m_bitrateSlider, &QSlider::setDisabled);
    connect(m_bitrateSlider, &QSlider::valueChanged, this, [this](int v){
        m_bitrateLabel->setText(AUDIO_BITRATE_LABELS[v]);
    });

    // スライダー目盛りラベル
    auto* tickRow = new QHBoxLayout;
    for (int i = 0; i < AUDIO_BITRATE_COUNT; i++)
        tickRow->addWidget(new QLabel(AUDIO_BITRATE_LABELS[i]));

    brLayout->addWidget(brCheck);
    brLayout->addWidget(m_bitrateSlider);
    brLayout->addWidget(m_bitrateLabel);
    brLayout->addLayout(tickRow);
    root->addWidget(brGroup);

    // ---- サンプリングレート ----
    auto* srGroup  = new QGroupBox("サンプリングレート", this);
    auto* srLayout = new QFormLayout(srGroup);
    m_sampleRateCombo = new QComboBox(this);
    m_sampleRateCombo->addItem("変更しない", -1);
    for (int i = 0; i < AUDIO_SAMPLERATE_COUNT; i++)
        m_sampleRateCombo->addItem(AUDIO_SAMPLERATE_LABELS[i], AUDIO_SAMPLE_RATES[i]);
    srLayout->addRow("サンプリングレート:", m_sampleRateCombo);

    m_bitDepthCombo = new QComboBox(this);
    m_bitDepthCombo->addItem("変更しない", -1);
    for (int i = 0; i < AUDIO_BITDEPTH_COUNT; i++)
        m_bitDepthCombo->addItem(AUDIO_BITDEPTH_LABELS[i], AUDIO_BIT_DEPTHS[i]);
    srLayout->addRow("ビット深度:", m_bitDepthCombo);
    root->addWidget(srGroup);

    root->addStretch();
}

void AudioSettingsWidget::applyToCompressOptions(CompressOptions& opts) const
{
    opts.audioQualityFirst = m_qualityFirst->isChecked();
    if (m_bitrateSlider->isEnabled())
        opts.audioBitrate = AUDIO_BITRATES[m_bitrateSlider->value()];
    opts.audioSampleRate = m_sampleRateCombo->currentData().toInt();
    opts.audioBitDepth   = m_bitDepthCombo->currentData().toInt();
}
