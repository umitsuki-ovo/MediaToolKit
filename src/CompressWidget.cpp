#include "CompressWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QScrollArea>

CompressWidget::CompressWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

void CompressWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8,8,8,8);
    root->setSpacing(8);

    // ---- 検出メディア種別ラベル ----
    auto* typeRow = new QHBoxLayout;
    typeRow->addWidget(new QLabel("対象:", this));
    m_typeLabel = new QLabel("ファイルを選択してください", this);
    m_typeLabel->setStyleSheet("color:#aaa; font-style:italic;");
    typeRow->addWidget(m_typeLabel);
    typeRow->addStretch();
    root->addLayout(typeRow);

    // ---- 設定パネル (スタック・自動切替) ----
    m_stack = new QStackedWidget(this);

    auto makeScroll = [](QWidget* w) -> QScrollArea* {
        auto* sc = new QScrollArea;
        sc->setWidget(w);
        sc->setWidgetResizable(true);
        sc->setFrameShape(QFrame::NoFrame);
        return sc;
    };

    // index 0: 未選択プレースホルダー
    auto* noneWidget = new QWidget(this);
    auto* noneLay    = new QVBoxLayout(noneWidget);
    auto* noneLbl    = new QLabel("ファイルを選択すると\n設定が表示されます", noneWidget);
    noneLbl->setAlignment(Qt::AlignCenter);
    noneLbl->setStyleSheet("color:#444; font-size:13px;");
    noneLay->addWidget(noneLbl);
    m_stack->addWidget(noneWidget);          // 0

    m_videoSettings = new VideoSettingsWidget(this);
    m_audioSettings = new AudioSettingsWidget(this);
    m_imageSettings = new ImageSettingsWidget(this);
    m_stack->addWidget(makeScroll(m_videoSettings));  // 1
    m_stack->addWidget(makeScroll(m_audioSettings));  // 2
    m_stack->addWidget(makeScroll(m_imageSettings));  // 3

    root->addWidget(m_stack, 1);

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
}

void CompressWidget::setMediaType(MediaType type)
{
    m_currentType = type;
    switch (type) {
        case MediaType::Video:
            m_typeLabel->setText("動画ファイル");
            m_typeLabel->setStyleSheet("color:#6ab0f5; font-weight:bold;");
            m_stack->setCurrentIndex(1);
            break;
        case MediaType::Audio:
            m_typeLabel->setText("音声ファイル");
            m_typeLabel->setStyleSheet("color:#a0d4a0; font-weight:bold;");
            m_stack->setCurrentIndex(2);
            break;
        case MediaType::Image:
            m_typeLabel->setText("画像ファイル");
            m_typeLabel->setStyleSheet("color:#f5c06a; font-weight:bold;");
            m_stack->setCurrentIndex(3);
            break;
        default:
            m_typeLabel->setText("ファイルを選択してください");
            m_typeLabel->setStyleSheet("color:#aaa; font-style:italic;");
            m_stack->setCurrentIndex(0);
            break;
    }
}

void CompressWidget::applyAppSettings(const AppSettings& s)
{
    m_appSettings = s;
    if (m_outputDirEdit->text().trimmed().isEmpty())
        m_outputDirEdit->setText(s.defaultOutputDir);
}

CompressOptions CompressWidget::buildOptions() const
{
    CompressOptions opts;
    // 全種別の設定を収集 (バッチ処理で混在ファイルに対応)
    m_videoSettings->applyToCompressOptions(opts);
    m_audioSettings->applyToCompressOptions(opts);
    m_imageSettings->applyToCompressOptions(opts);
    opts.useGPU    = m_appSettings.gpuEncode();
    opts.gpuType   = m_appSettings.gpuType;
    QString dir    = m_outputDirEdit->text().trimmed();
    opts.outputDir = dir.isEmpty() ? m_appSettings.defaultOutputDir : dir;
    return opts;
}
