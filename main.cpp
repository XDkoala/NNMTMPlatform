// main.cpp
#include <QApplication>
#include "AppUI.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // 设置全局字体和样式
    QFont font("Microsoft YaHei", 9);
    a.setFont(font);

    MainWindow w;
    w.show();

    return a.exec();
}