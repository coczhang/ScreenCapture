#include <QApplication>
#include <QDateTime>
#include <QDebug>

#include "screen_capture.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::shared_ptr<ScreenCapture> screen_capture = std::make_shared<ScreenCapture>();
    if (!screen_capture.get()->InitD3D11Device()) {
        return  -1;
    }

    if (!screen_capture.get()->InitDuplication()) {
        return -1;
    }

    int num = 0;
    /* 每隔1秒获取一次图像 */
    while (num < 5) {
        QString fileName = QString("%1.png").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss"));
        qDebug() << fileName;
        screen_capture.get()->GetDesktopFrame(fileName);

        num++;
        Sleep(1000);
    }

    return a.exec();
}
