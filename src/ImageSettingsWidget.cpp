#include "ImageSettingsWidget.h"
#include "MediaTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

ImageSettingsWidget::ImageSettingsWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

void ImageSettingsWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(8);

    // ---- 圧縮方式 ----
    auto* typeGroup  = new QGroupBox("圧縮方式", this);
    auto* typeLayout = new QVBoxLayout(typeGroup);
    m_losslessCheck = new QCheckBox("可逆圧縮 (PNG等)", this);
    typeLayout->addWidget(m_losslessCheck);
    root->addWidget(typeGroup);

    // ---- 品質 (非可逆) ----
    auto* qualGroup  = new QGroupBox("品質 (非可逆圧縮)", this);
    auto* qualLayout = new QVBoxLayout(qualGroup);

    m_qualitySlider = new QSlider(Qt::Horizontal, this);
    m_qualitySlider->setRange(1, 100);
    m_qualitySlider->setValue(85);
    m_qualitySlider->setTickInterval(10);
    m_qualitySlider->setTickPosition(QSlider::TicksBelow);

    // 目盛りラベル
    auto* tickRow = new QHBoxLayout;
    tickRow->addWidget(new QLabel("低", this));
    tickRow->addStretch();
    tickRow->addWidget(new QLabel("中", this));
    tickRow->addStretch();
    tickRow->addWidget(new QLabel("高", this));

    // 値 + レベル表示行
    auto* valRow = new QHBoxLayout;
    m_qualityLabel   = new QLabel("85", this);
    m_qualityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_qualityLabel->setMinimumWidth(28);
    m_qualLevelLabel = new QLabel("高品質", this);
    m_qualLevelLabel->setStyleSheet("color:#4caf50; font-weight:bold; margin-left:6px;");
    valRow->addStretch();
    valRow->addWidget(m_qualityLabel);
    valRow->addWidget(m_qualLevelLabel);

    qualLayout->addWidget(m_qualitySlider);
    qualLayout->addLayout(tickRow);
    qualLayout->addLayout(valRow);

    connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int v) {
        m_qualityLabel->setText(QString::number(v));
        if (v <= 40) {
            m_qualLevelLabel->setText("低品質");
            m_qualLevelLabel->setStyleSheet("color:#f44336; font-weight:bold; margin-left:6px;");
        } else if (v <= 70) {
            m_qualLevelLabel->setText("中品質");
            m_qualLevelLabel->setStyleSheet("color:#ff9800; font-weight:bold; margin-left:6px;");
        } else {
            m_qualLevelLabel->setText("高品質");
            m_qualLevelLabel->setStyleSheet("color:#4caf50; font-weight:bold; margin-left:6px;");
        }
    });

    connect(m_losslessCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_qualitySlider->setEnabled(!checked);
        m_targetSizeCheck->setEnabled(!checked);
        if (checked) m_targetSizeBox->setEnabled(false);
    });

    root->addWidget(qualGroup);

    // ---- ファイルサイズ指定圧縮 ----
    auto* sizeGroup  = new QGroupBox("ファイルサイズ指定 (JPEG)", this);
    auto* sizeLayout = new QHBoxLayout(sizeGroup);
    m_targetSizeCheck = new QCheckBox("目標サイズ:", this);
    m_targetSizeBox   = new QSpinBox(this);
    m_targetSizeBox->setRange(1, 100000);
    m_targetSizeBox->setValue(500);
    m_targetSizeBox->setSuffix(" KB");
    m_targetSizeBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_targetSizeBox->setEnabled(false);  // チェックされるまで無効

    connect(m_targetSizeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_targetSizeBox->setEnabled(checked);
    });

    sizeLayout->addWidget(m_targetSizeCheck);
    sizeLayout->addWidget(m_targetSizeBox);
    sizeLayout->addStretch();
    root->addWidget(sizeGroup);

    // ---- 解像度変更 ----
    auto* resGroup  = new QGroupBox("解像度変更", this);
    auto* resLayout = new QVBoxLayout(resGroup);
    m_resCombo = new QComboBox(this);
    m_resCombo->addItem("変更しない", -1);

    static const struct { int w; int h; const char* lbl; } IMAGE_RES[] = {
        {3840, 2160, "4K (3840x2160)"},
        {1920, 1080, "FHD (1920x1080)"},
        {1280,  720, "HD (1280x720)"},
        {1024,  768, "XGA (1024x768)"},
        { 800,  600, "SVGA (800x600)"},
        { 640,  480, "VGA (640x480)"},
        { 480,  270, "480x270"},
        { 320,  240, "320x240"},
    };
    for (const auto& r : IMAGE_RES)
        m_resCombo->addItem(r.lbl, r.w * 10000 + r.h);
    m_resCombo->addItem("カスタム", 0);

    auto* customRow = new QHBoxLayout;
    m_customW = new QSpinBox(this);
    m_customH = new QSpinBox(this);
    m_customW->setRange(1, 9999); m_customW->setSuffix(" px"); m_customW->setValue(1920);
    m_customH->setRange(1, 9999); m_customH->setSuffix(" px"); m_customH->setValue(1080);
    m_customW->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_customH->setButtonSymbols(QAbstractSpinBox::NoButtons);
    customRow->addWidget(new QLabel("W:"));
    customRow->addWidget(m_customW);
    customRow->addWidget(new QLabel("H:"));
    customRow->addWidget(m_customH);

    auto* custWidget = new QWidget(this);
    custWidget->setLayout(customRow);
    custWidget->hide();

    connect(m_resCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, custWidget](int) {
        custWidget->setVisible(m_resCombo->currentData().toInt() == 0);
    });

    resLayout->addWidget(m_resCombo);
    resLayout->addWidget(custWidget);
    root->addWidget(resGroup);

    root->addStretch();
}

void ImageSettingsWidget::applyToCompressOptions(CompressOptions& opts) const
{
    opts.imageLossless = m_losslessCheck->isChecked();
    opts.imageQuality  = m_qualitySlider->value();
    opts.targetSizeKB  = (m_targetSizeCheck->isChecked() && m_targetSizeBox->isEnabled())
                         ? m_targetSizeBox->value() : -1;

    int rv = m_resCombo->currentData().toInt();
    if (rv > 0) {
        opts.imageWidth  = rv / 10000;
        opts.imageHeight = rv % 10000;
    } else if (rv == 0) {
        opts.imageWidth  = m_customW->value();
        opts.imageHeight = m_customH->value();
    }
}
