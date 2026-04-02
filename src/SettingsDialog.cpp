#include "SettingsDialog.h"
#include "FFmpegProcessor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFileDialog>
#include <QThread>
#include <QSettings>

static const char* ORG = "MediaToolKit";
static const char* APP = "MediaToolKit";

// ================================================================
// 永続化
// ================================================================
AppSettings SettingsDialog::loadSettings()
{
    QSettings s(ORG, APP);
    AppSettings a;
    a.gpuMode             = (GpuMode)s.value("gpu/mode",    0).toInt();
    a.gpuType             = s.value("gpu/type",          "nvenc").toString();
    a.defaultOutputDir    = s.value("output/dir",        "").toString();
    a.threads             = s.value("process/threads",   0).toInt();
    a.defaultImageQuality = s.value("image/quality",     85).toInt();
    return a;
}

void SettingsDialog::saveSettings(const AppSettings& a)
{
    QSettings s(ORG, APP);
    s.setValue("gpu/mode",        (int)a.gpuMode);
    s.setValue("gpu/type",        a.gpuType);
    s.setValue("output/dir",      a.defaultOutputDir);
    s.setValue("process/threads", a.threads);
    s.setValue("image/quality",   a.defaultImageQuality);
    s.sync();
}

// ================================================================
// コンストラクタ
// ================================================================
SettingsDialog::SettingsDialog(const AppSettings& current, QWidget* parent)
    : QDialog(parent), m_initial(current)
{
    setWindowTitle("設定");
    setMinimumWidth(480);
    setModal(true);
    setupUi();
}

void SettingsDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);

    // ---- GPU エンコード ----
    auto* gpuGroup  = new QGroupBox("GPU エンコード", this);
    auto* gpuLayout = new QVBoxLayout(gpuGroup);

    // GPU モード選択
    auto* modeRow = new QHBoxLayout;
    modeRow->addWidget(new QLabel("エンコードモード:", this));
    m_gpuModeCombo = new QComboBox(this);
    m_gpuModeCombo->addItem("CPU のみ (GPU 不使用)",       (int)GpuMode::Disabled);
    m_gpuModeCombo->addItem("CPU デコード + GPU エンコード (推奨)", (int)GpuMode::CpuAndGpu);
    m_gpuModeCombo->addItem("GPU エンコードのみ",           (int)GpuMode::GpuOnly);
    int modeIdx = m_gpuModeCombo->findData((int)m_initial.gpuMode);
    if (modeIdx >= 0) m_gpuModeCombo->setCurrentIndex(modeIdx);
    modeRow->addWidget(m_gpuModeCombo, 1);
    gpuLayout->addLayout(modeRow);

    // GPU 種別
    auto* typeRow = new QHBoxLayout;
    typeRow->addWidget(new QLabel("GPU エンコーダ:", this));
    m_gpuTypeCombo = new QComboBox(this);
    m_gpuTypeCombo->addItem("NVIDIA NVENC", "nvenc");
    m_gpuTypeCombo->addItem("Intel QSV",    "qsv");
    m_gpuTypeCombo->addItem("AMD AMF",      "amf");
    int tidx = m_gpuTypeCombo->findData(m_initial.gpuType);
    if (tidx >= 0) m_gpuTypeCombo->setCurrentIndex(tidx);
    typeRow->addWidget(m_gpuTypeCombo, 1);

    // 利用可否確認
    auto* checkBtn  = new QPushButton("確認", this);
    auto* statusLbl = new QLabel(this);
    statusLbl->setWordWrap(true);
    statusLbl->setStyleSheet("color:#888; font-size:11px;");
    connect(checkBtn, &QPushButton::clicked, this, [this, statusLbl]{
        QString type = m_gpuTypeCombo->currentData().toString();
        bool ok = FFmpegProcessor::isGpuEncoderAvailable(type);
        statusLbl->setText(ok ? "利用可能です" : "このGPUは利用できません (ドライバ/FFmpegを確認)");
        statusLbl->setStyleSheet(ok ? "color:#6abf6a; font-size:11px;"
                                    : "color:#f08080; font-size:11px;");
    });
    typeRow->addWidget(checkBtn);
    gpuLayout->addLayout(typeRow);
    gpuLayout->addWidget(statusLbl);

    // モードが CPU のみ のとき GPU 種別を無効化
    auto updateGpuTypeEnabled = [this]() {
        bool gpuEnabled = m_gpuModeCombo->currentData().toInt() != (int)GpuMode::Disabled;
        m_gpuTypeCombo->setEnabled(gpuEnabled);
    };
    connect(m_gpuModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [updateGpuTypeEnabled](int){ updateGpuTypeEnabled(); });
    updateGpuTypeEnabled();

    // 説明ラベル
    auto* gpuNote = new QLabel(
        "「CPU + GPU」モード: CPU でデコード → GPU でエンコード\n"
        "GPU が利用できない場合は自動で CPU にフォールバックします。", this);
    gpuNote->setStyleSheet("color:#888; font-size:11px;");
    gpuLayout->addWidget(gpuNote);
    root->addWidget(gpuGroup);

    // ---- 出力先 ----
    auto* outGroup  = new QGroupBox("デフォルト出力先フォルダ", this);
    auto* outLayout = new QVBoxLayout(outGroup);
    auto* outNote   = new QLabel("空白の場合は入力ファイルと同じフォルダに出力します。", this);
    outNote->setStyleSheet("color:#888; font-size:11px;");
    auto* outRow    = new QHBoxLayout;
    m_outputDirEdit = new QLineEdit(m_initial.defaultOutputDir, this);
    m_outputDirEdit->setPlaceholderText("(空白 = 入力ファイルと同じ場所)");
    m_browseDirBtn  = new QPushButton("参照...", this);
    connect(m_browseDirBtn, &QPushButton::clicked, this, [this]{
        QString dir = QFileDialog::getExistingDirectory(this, "出力先フォルダ", m_outputDirEdit->text());
        if (!dir.isEmpty()) m_outputDirEdit->setText(dir);
    });
    auto* clearBtn = new QPushButton("クリア", this);
    connect(clearBtn, &QPushButton::clicked, m_outputDirEdit, &QLineEdit::clear);
    outRow->addWidget(m_outputDirEdit, 1);
    outRow->addWidget(m_browseDirBtn);
    outRow->addWidget(clearBtn);
    outLayout->addWidget(outNote);
    outLayout->addLayout(outRow);
    root->addWidget(outGroup);

    // ---- スレッド数 ----
    auto* thrGroup  = new QGroupBox("処理スレッド数 (バッチ: デコード・エンコード)", this);
    auto* thrLayout = new QFormLayout(thrGroup);
    m_threadsSpin = new QSpinBox(this);
    m_threadsSpin->setRange(0, 64);
    m_threadsSpin->setValue(m_initial.threads);
    m_threadsSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_threadsSpin->setSpecialValueText(
        QString("自動 (%1 スレッド)").arg(QThread::idealThreadCount()));
    thrLayout->addRow("コーデックスレッド数:", m_threadsSpin);
    auto* thrNote = new QLabel("0 = FFmpeg が自動設定 (推奨)", this);
    thrNote->setStyleSheet("color:#888; font-size:11px;");
    thrLayout->addRow("", thrNote);
    root->addWidget(thrGroup);

    // ---- 画像品質 ----
    auto* qualGroup  = new QGroupBox("デフォルト画像品質", this);
    auto* qualLayout = new QFormLayout(qualGroup);
    m_qualitySpin = new QSpinBox(this);
    m_qualitySpin->setRange(1, 100);
    m_qualitySpin->setValue(m_initial.defaultImageQuality);
    m_qualitySpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    qualLayout->addRow("JPEG/WebP 品質 (1-100):", m_qualitySpin);
    root->addWidget(qualGroup);

    // ---- OK/Cancel ----
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

AppSettings SettingsDialog::getSettings() const
{
    AppSettings s;
    s.gpuMode             = (GpuMode)m_gpuModeCombo->currentData().toInt();
    s.gpuType             = m_gpuTypeCombo->currentData().toString();
    s.defaultOutputDir    = m_outputDirEdit->text().trimmed();
    s.threads             = m_threadsSpin->value();
    s.defaultImageQuality = m_qualitySpin->value();
    return s;
}
