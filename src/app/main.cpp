// main.cpp
#include <QApplication>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include "AppUI.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // A packaged build stores data beside the executable, even from a shortcut.
    if (QFileInfo::exists(a.applicationDirPath() + "/portable.ini")) {
        if (!QDir::setCurrent(a.applicationDirPath())) return 1;
    }

    // 设置全局字体和样式
    QFont font("Microsoft YaHei", 9);
    a.setFont(font);

    MainWindow w;
    w.show();

    // Packaging smoke check: start the real UI and exit through Qt normally.
    if (a.arguments().contains("--smoke-test")) {
        QTimer::singleShot(1500, &a, &QCoreApplication::quit);
    }

    return a.exec();
}
