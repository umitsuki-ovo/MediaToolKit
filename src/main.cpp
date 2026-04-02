#include <QApplication>
#include <QImageReader>
#include <QImageWriter>
#include <QDir>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("MediaToolKit");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MyOrg");

    // アプリアイコン (resources.qrc から)
    app.setWindowIcon(QIcon(":/icons/app.ico"));

    // Windows フォント
    QFont font("Yu Gothic UI");
    font.setPointSize(10);
    app.setFont(font);

    // ★ imageformats プラグインの検索パスを追加
    //   windeployqt で {exe}/imageformats/ にコピーされた DLL を認識させる
    QCoreApplication::addLibraryPath(
        QCoreApplication::applicationDirPath());

    MainWindow window;
    window.show();

    return app.exec();
}
