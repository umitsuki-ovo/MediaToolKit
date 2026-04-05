#include "AudioSettingsWidget.h"
#include "MediaTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>

AudioSettingsWidget::AudioSettingsWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

void AudioSettingsWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(8);

    // ---- 最適化モード (カード型ボタン) ----
    auto* modeGroup  = new QGroupBox("最適化モード", this);
    auto* modeLayout = new QHBoxLayout(modeGroup);
    modeLayout->setSpacing(8);

    m_qualityFirst = new QRadioButton(this);
    m_sizeFirst    = new QRadioButton(this);
    m_qualityFirst->setChecked(true);
    // ラジオボタン自体は非表示にし、カスタム外観のボタンで代替
    m_qualityFirst->hide();
    m_sizeFirst->hide();

    // カード型の選択ボタン
    auto* qualBtn = new QPushButton("品質優先\n高品質・大きいサイズ", modeGroup);
    auto* sizeBtn = new QPushButton("サイズ優先\n小さいサイズ・低品質", modeGroup);
    qualBtn->setObjectName("modeCard");
    sizeBtn->setObjectName("modeCard");
    qualBtn->setCheckable(true);
    sizeBtn->setCheckable(true);
    qualBtn->setChecked(true);
    qualBtn->setMinimumHeight(56);
    sizeBtn->setMinimumHeight(56);

    auto* btnGroup = new QButtonGroup(modeGroup);
    btnGroup->addButton(qualBtn);
    btnGroup->addButton(sizeBtn);
    btnGroup->setExclusive(true);

    // カードボタンとラジオボタンを連動
    connect(qualBtn, &QPushButton::clicked, this, [this]{ m_qualityFirst->setChecked(true); });
    connect(sizeBtn, &QPushButton::clicked, this, [this]{ m_sizeFirst->setChecked(true); });

    // スタイル
    auto cardStyle = R"(
        QPushButton#modeCard {
            background: #252525;
            color: #999;
            border: 2px solid #3a3a3a;
            border-radius: 6px;
            padding: 8px;
            font-size: 12px;
            text-align: center;
        }
        QPushButton#modeCard:checked {
            background: #1a3a5c;
            color: #fff;
            border: 2px solid #2d5a8e;
            font-weight: bold;
        }
        QPushButton#modeCard:hover:!checked {
            background: #2e2e2e;
            border-color: #555;
            color: #ccc;
        }
    )";
    qualBtn->setStyleSheet(cardStyle);
    sizeBtn->setStyleSheet(cardStyle);

    modeLayout->addWidget(qualBtn);
    modeLayout->addWidget(sizeBtn);
    root->addWidget(modeGroup);

    // ---- ビットレート ----
    auto* brGroup  = new QGroupBox("ビットレート", this);
    auto* brLayout = new QVBoxLayout(brGroup);
    auto* brCheck  = new QCheckBox("変更しない", this);
    brCheck->setChecked(true);

    m_bitrateSlider = new QSlider(Qt::Horizontal, this);
    m_bitrateSlider->setRange(0, AUDIO_BITRATE_COUNT - 1);
    m_bitrateSlider->setValue(2);
    m_bitrateSlider->setTickPosition(QSlider::TicksBelow);
    m_bitrateSlider->setEnabled(false);

    m_bitrateLabel = new QLabel(AUDIO_BITRATE_LABELS[2], this);
    m_bitrateLabel->setAlignment(Qt::AlignCenter);

    connect(brCheck, &QCheckBox::toggled, m_bitrateSlider, &QSlider::setDisabled);
    connect(m_bitrateSlider, &QSlider::valueChanged, this, [this](int v){
        m_bitrateLabel->setText(AUDIO_BITRATE_LABELS[v]);
    });

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
