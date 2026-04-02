#include "ConvertWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QScrollArea>

ConvertWidget::ConvertWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

void ConvertWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8,8,8,8);
    root->setSpacing(8);

    // ---- 変換設定 ----
    auto* convGroup  = new QGroupBox("変換設定", this);
    auto* form       = new QFormLayout(convGroup);
    form->setRowWrapPolicy(QFormLayout::DontWrapRows);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);

    // 入力種別
    m_typeLabel = new QLabel("ファイルを選択してください", this);
    m_typeLabel->setStyleSheet("color:#aaa; font-style:italic;");
    form->addRow("入力種別:", m_typeLabel);

    // 変換タイプ (動画のみ)
    m_targetCombo = new QComboBox(this);
    m_targetRow   = new QWidget(this);
    {
        auto* lay = new QHBoxLayout(m_targetRow);
        lay->setContentsMargins(0,0,0,0);
        lay->addWidget(m_targetCombo);
        lay->addStretch();
    }
    form->addRow("変換タイプ:", m_targetRow);
    m_targetRow->hide();

    // 出力フォーマット
    m_formatCombo = new QComboBox(this);
    form->addRow("出力フォーマット:", m_formatCombo);

    // コーデック (動画→動画のみ)
    m_codecCombo = new QComboBox(this);
    m_codecCombo->addItem("H.264",        (int)VideoCodec::H264);
    m_codecCombo->addItem("H.265 (HEVC)", (int)VideoCodec::H265);
    m_codecCombo->addItem("MPEG-4",       (int)VideoCodec::MPEG4);
    m_codecRow = new QWidget(this);
    {
        auto* lay = new QHBoxLayout(m_codecRow);
        lay->setContentsMargins(0,0,0,0);
        lay->addWidget(m_codecCombo);
        lay->addStretch();
    }
    form->addRow("コーデック:", m_codecRow);
    m_codecRow->hide();

    // フレーム指定 (動画→画像のみ)
    m_frameModeCombo = new QComboBox(this);
    m_frameModeCombo->addItem("時間 (秒)");
    m_frameModeCombo->addItem("フレーム番号");
    m_frameTimeSpin = new QSpinBox(this);
    m_frameTimeSpin->setRange(0, 999999);
    m_frameTimeSpin->setSuffix(" 秒");
    m_frameTimeSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_frameNumSpin  = new QSpinBox(this);
    m_frameNumSpin->setRange(0, 9999999);
    m_frameNumSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_frameNumSpin->hide();

    m_frameRow = new QWidget(this);
    {
        auto* lay = new QHBoxLayout(m_frameRow);
        lay->setContentsMargins(0,0,0,0);
        lay->addWidget(m_frameModeCombo);
        lay->addWidget(m_frameTimeSpin);
        lay->addWidget(m_frameNumSpin);
        lay->addStretch();
    }
    form->addRow("フレーム指定:", m_frameRow);
    m_frameRow->hide();

    connect(m_frameModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int i) {
        m_frameTimeSpin->setVisible(i == 0);
        m_frameNumSpin->setVisible(i == 1);
    });

    root->addWidget(convGroup);

    // ---- 出力フォルダ ----
    auto* outGroup  = new QGroupBox("出力先フォルダ", this);
    auto* outLayout = new QHBoxLayout(outGroup);
    m_outputDirEdit = new QLineEdit(this);
    m_outputDirEdit->setPlaceholderText("空白の場合はデフォルト設定を使用");
    m_browseDirBtn  = new QPushButton("参照...", this);
    connect(m_browseDirBtn, &QPushButton::clicked, this, [this]{
        QString dir = QFileDialog::getExistingDirectory(this, "出力先フォルダを選択");
        if (!dir.isEmpty()) m_outputDirEdit->setText(dir);
    });
    outLayout->addWidget(m_outputDirEdit);
    outLayout->addWidget(m_browseDirBtn);
    root->addWidget(outGroup);

    root->addStretch();

    connect(m_targetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConvertWidget::onConvertTargetChanged);
}

void ConvertWidget::applyAppSettings(const AppSettings& s)
{
    m_appSettings = s;
    if (m_outputDirEdit->text().trimmed().isEmpty())
        m_outputDirEdit->setText(s.defaultOutputDir);
}

void ConvertWidget::setSourceMediaType(MediaType type)
{
    // ★ 早期リターンなし: モード切替後の再選択でも必ず更新する
    m_sourceType = type;

    switch (type) {
        case MediaType::Video:
            m_typeLabel->setText("動画ファイル");
            m_typeLabel->setStyleSheet("color:#6ab0f5; font-weight:bold;");
            break;
        case MediaType::Audio:
            m_typeLabel->setText("音声ファイル");
            m_typeLabel->setStyleSheet("color:#a0d4a0; font-weight:bold;");
            break;
        case MediaType::Image:
            m_typeLabel->setText("画像ファイル");
            m_typeLabel->setStyleSheet("color:#f5c06a; font-weight:bold;");
            break;
        default:
            m_typeLabel->setText("ファイルを選択してください");
            m_typeLabel->setStyleSheet("color:#aaa; font-style:italic;");
            break;
    }

    {
        QSignalBlocker b(m_targetCombo);
        m_targetCombo->clear();
        if (type == MediaType::Video) {
            m_targetCombo->addItem("動画 → 動画",                (int)ConvertTarget::VideoToVideo);
            m_targetCombo->addItem("動画 → 音声 (音声抽出)",     (int)ConvertTarget::VideoToAudio);
            m_targetCombo->addItem("動画 → 画像 (フレーム抽出)", (int)ConvertTarget::VideoToImage);
            m_targetRow->show();
        } else {
            if (type == MediaType::Audio)
                m_targetCombo->addItem("音声 → 音声", (int)ConvertTarget::AudioToAudio);
            else if (type == MediaType::Image)
                m_targetCombo->addItem("画像 → 画像", (int)ConvertTarget::ImageToImage);
            m_targetRow->hide();
        }
    }
    onConvertTargetChanged(0);
}

void ConvertWidget::onConvertTargetChanged(int /*index*/)
{
    m_currentTarget = (ConvertTarget)m_targetCombo->currentData().toInt();

    // コーデック: 動画→動画のみ
    m_codecRow->setVisible(m_currentTarget == ConvertTarget::VideoToVideo);
    // フレーム指定: 動画→画像のみ
    m_frameRow->setVisible(m_currentTarget == ConvertTarget::VideoToImage);

    updateFormatCombo();
}

void ConvertWidget::updateFormatCombo()
{
    QSignalBlocker b(m_formatCombo);
    m_formatCombo->clear();

    QStringList fmts;
    switch (m_currentTarget) {
        case ConvertTarget::VideoToVideo:  fmts = VIDEO_OUTPUT_FORMATS;   break;
        case ConvertTarget::VideoToAudio:  fmts = VIDEO_TO_AUDIO_FORMATS; break;
        case ConvertTarget::VideoToImage:  fmts = VIDEO_TO_IMAGE_FORMATS; break;
        case ConvertTarget::AudioToAudio:  fmts = AUDIO_OUTPUT_FORMATS;   break;
        case ConvertTarget::ImageToImage:  fmts = IMAGE_OUTPUT_FORMATS;   break;
    }
    for (const QString& f : fmts)
        m_formatCombo->addItem(f.toUpper(), f);
}

ConvertOptions ConvertWidget::buildOptions() const
{
    ConvertOptions opts;
    opts.target    = m_currentTarget;
    opts.format    = m_formatCombo->currentData().toString();
    opts.useGPU    = m_appSettings.gpuEncode();
    opts.gpuType   = m_appSettings.gpuType;
    QString dir    = m_outputDirEdit->text().trimmed();
    opts.outputDir = dir.isEmpty() ? m_appSettings.defaultOutputDir : dir;

    if (m_currentTarget == ConvertTarget::VideoToVideo)
        opts.videoCodec = (VideoCodec)m_codecCombo->currentData().toInt();

    if (m_currentTarget == ConvertTarget::VideoToImage) {
        if (m_frameModeCombo->currentIndex() == 0) {
            opts.frameTimeSec = m_frameTimeSpin->value();
            opts.frameNumber  = -1;
        } else {
            opts.frameNumber  = m_frameNumSpin->value();
            opts.frameTimeSec = -1;
        }
    }
    return opts;
}
