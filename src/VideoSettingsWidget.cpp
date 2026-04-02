#include "VideoSettingsWidget.h"
#include "MediaTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

VideoSettingsWidget::VideoSettingsWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

void VideoSettingsWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);

    // ---- 解像度グループ ----
    auto* resGroup = new QGroupBox("解像度", this);
    auto* resLayout = new QVBoxLayout(resGroup);

    m_resCombo = new QComboBox(this);
    m_resCombo->addItem("変更しない", -1);
    for (int i = 0; i < VIDEO_RES_COUNT; i++)
        m_resCombo->addItem(VIDEO_RESOLUTIONS[i].label, i);
    m_resCombo->addItem("カスタム", 99);

    auto* customRow = new QHBoxLayout;
    m_customW = new QSpinBox(this);
    m_customH = new QSpinBox(this);
    m_customW->setRange(1, 7680); m_customW->setSuffix(" px"); m_customW->setValue(1920);
    m_customH->setRange(1, 4320); m_customH->setSuffix(" px"); m_customH->setValue(1080);
    m_customW->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_customH->setButtonSymbols(QAbstractSpinBox::NoButtons);
    customRow->addWidget(new QLabel("W:"));
    customRow->addWidget(m_customW);
    customRow->addWidget(new QLabel("H:"));
    customRow->addWidget(m_customH);

    auto* custWidget = new QWidget;
    custWidget->setLayout(customRow);
    custWidget->hide();

    resLayout->addWidget(m_resCombo);
    resLayout->addWidget(custWidget);

    connect(m_resCombo, &QComboBox::currentIndexChanged, this, [this, custWidget](int idx){
        custWidget->setVisible(m_resCombo->currentData().toInt() == 99);
    });

    root->addWidget(resGroup);

    // ---- FPS グループ ----
    auto* fpsGroup  = new QGroupBox("フレームレート (fps)", this);
    auto* fpsLayout = new QVBoxLayout(fpsGroup);

    m_fpsSlider = new QSlider(Qt::Horizontal, this);
    m_fpsSlider->setRange(0, FPS_COUNT - 1);
    m_fpsSlider->setValue(4); // 30fps
    m_fpsSlider->setTickPosition(QSlider::TicksBelow);
    m_fpsSlider->setTickInterval(1);

    m_fpsLabel = new QLabel(FPS_LABELS[4] + " fps", this);
    m_fpsLabel->setAlignment(Qt::AlignCenter);

    auto* fpsCheck = new QCheckBox("変更しない", this);
    fpsCheck->setChecked(true);
    connect(fpsCheck, &QCheckBox::toggled, this, [this](bool checked){
        m_fpsSlider->setEnabled(!checked);
    });
    m_fpsSlider->setEnabled(false);

    fpsLayout->addWidget(fpsCheck);
    fpsLayout->addWidget(m_fpsSlider);
    fpsLayout->addWidget(m_fpsLabel);

    connect(m_fpsSlider, &QSlider::valueChanged, this, [this](int v){
        m_fpsLabel->setText(FPS_LABELS[v] + " fps");
    });

    root->addWidget(fpsGroup);

    // ---- コーデック (MP4) ----
    auto* codecGroup  = new QGroupBox("コーデック (MP4)", this);
    auto* codecLayout = new QFormLayout(codecGroup);
    m_codecCombo = new QComboBox(this);
    m_codecCombo->addItem("H.264",  (int)VideoCodec::H264);
    m_codecCombo->addItem("H.265 (HEVC)", (int)VideoCodec::H265);
    m_codecCombo->addItem("MPEG-4", (int)VideoCodec::MPEG4);
    codecLayout->addRow("コーデック:", m_codecCombo);
    root->addWidget(codecGroup);

    // ---- フレーム抽出 (変換: 動画→画像) ----
    m_frameWidget = new QGroupBox("フレーム抽出", this);
    auto* frameLayout = new QFormLayout(m_frameWidget);
    m_frameMode = new QComboBox(this);
    m_frameMode->addItem("時間指定 (秒)");
    m_frameMode->addItem("フレーム番号");
    m_frameTime = new QSpinBox(this);
    m_frameTime->setRange(0, 99999); m_frameTime->setSuffix(" 秒");
    m_frameTime->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_frameNum  = new QSpinBox(this);
    m_frameNum->setRange(0, 9999999);
    m_frameNum->setButtonSymbols(QAbstractSpinBox::NoButtons);
    frameLayout->addRow("指定方法:", m_frameMode);
    frameLayout->addRow("時刻:", m_frameTime);
    frameLayout->addRow("フレーム#:", m_frameNum);
    connect(m_frameMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i){
        m_frameTime->setVisible(i == 0);
        m_frameNum->setVisible(i == 1);
    });
    m_frameNum->hide();
    m_frameWidget->hide();

    root->addWidget(m_frameWidget);
    root->addStretch();
}

void VideoSettingsWidget::setConvertTarget(ConvertTarget target)
{
    m_convertTarget = target;
    bool isFrameExtract = (target == ConvertTarget::VideoToImage);
    m_frameWidget->setVisible(isFrameExtract);
}

void VideoSettingsWidget::applyToCompressOptions(CompressOptions& opts) const
{
    int resIdx = m_resCombo->currentData().toInt();
    if (resIdx >= 0 && resIdx < VIDEO_RES_COUNT) {
        opts.videoWidth  = VIDEO_RESOLUTIONS[resIdx].width;
        opts.videoHeight = VIDEO_RESOLUTIONS[resIdx].height;
    } else if (resIdx == 99) {
        opts.videoWidth  = m_customW->value();
        opts.videoHeight = m_customH->value();
    }

    bool fpsEnabled = m_fpsSlider->isEnabled();
    if (fpsEnabled) opts.videoFps = FPS_VALUES[m_fpsSlider->value()];

    opts.videoCodec = (VideoCodec)m_codecCombo->currentData().toInt();
}

void VideoSettingsWidget::applyToConvertOptions(ConvertOptions& opts) const
{
    opts.videoCodec = (VideoCodec)m_codecCombo->currentData().toInt();
    if (m_convertTarget == ConvertTarget::VideoToImage) {
        opts.target = ConvertTarget::VideoToImage;
        if (m_frameMode->currentIndex() == 0) {
            opts.frameTimeSec = m_frameTime->value();
            opts.frameNumber  = -1;
        } else {
            opts.frameNumber  = m_frameNum->value();
            opts.frameTimeSec = -1;
        }
    }
}
