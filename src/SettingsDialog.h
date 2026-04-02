#pragma once
#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>

// GPU 使用モード
enum class GpuMode {
    Disabled = 0,   // GPU 使用しない (CPU のみ)
    GpuOnly,        // GPU エンコードのみ
    CpuAndGpu,      // CPU デコード + GPU エンコード (推奨)
};

struct AppSettings {
    GpuMode gpuMode            = GpuMode::Disabled;
    QString gpuType            = "nvenc";   // "nvenc" / "qsv" / "amf"
    QString defaultOutputDir;
    int     threads            = 0;         // 0 = 自動
    int     defaultImageQuality= 85;

    // 後方互換用
    bool useGpu() const { return gpuMode != GpuMode::Disabled; }
    bool gpuEncode() const { return gpuMode == GpuMode::GpuOnly || gpuMode == GpuMode::CpuAndGpu; }
};

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const AppSettings& current, QWidget* parent = nullptr);

    AppSettings getSettings() const;

    static AppSettings loadSettings();
    static void        saveSettings(const AppSettings& s);

private:
    void setupUi();

    QComboBox*   m_gpuModeCombo  = nullptr;
    QComboBox*   m_gpuTypeCombo  = nullptr;
    QLineEdit*   m_outputDirEdit = nullptr;
    QPushButton* m_browseDirBtn  = nullptr;
    QSpinBox*    m_threadsSpin   = nullptr;
    QSpinBox*    m_qualitySpin   = nullptr;

    AppSettings m_initial;
};
