#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QFormLayout>
#include "FFmpegProcessor.h"
#include "MediaTypes.h"
#include "SettingsDialog.h"

class ConvertWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConvertWidget(QWidget* parent = nullptr);

    void setSourceMediaType(MediaType type);
    void applyAppSettings(const AppSettings& s);
    ConvertOptions buildOptions() const;

private slots:
    void onConvertTargetChanged(int index);

private:
    void setupUi();
    void updateFormatCombo();

    MediaType     m_sourceType    = MediaType::Unknown;
    ConvertTarget m_currentTarget = ConvertTarget::VideoToVideo;

    // 入力種別ラベル
    QLabel*    m_typeLabel     = nullptr;

    // 変換タイプ (動画のみ表示)
    QWidget*   m_targetRow     = nullptr;
    QComboBox* m_targetCombo   = nullptr;

    // 出力フォーマット
    QComboBox* m_formatCombo   = nullptr;

    // コーデック (動画→動画のみ表示)
    QWidget*   m_codecRow      = nullptr;
    QComboBox* m_codecCombo    = nullptr;

    // フレーム指定 (動画→画像のみ表示)
    QWidget*   m_frameRow      = nullptr;
    QComboBox* m_frameModeCombo= nullptr;
    QSpinBox*  m_frameTimeSpin = nullptr;
    QSpinBox*  m_frameNumSpin  = nullptr;

    // 出力フォルダ
    QLineEdit*   m_outputDirEdit = nullptr;
    QPushButton* m_browseDirBtn  = nullptr;

    AppSettings  m_appSettings;
};
