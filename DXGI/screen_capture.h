#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QObject>
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <QImage>

class ScreenCapture : public QObject
{
    Q_OBJECT
public:
    explicit ScreenCapture(QObject *parent = nullptr);
    ~ScreenCapture();

public:
    bool InitD3D11Device();
    bool InitDuplication();
    bool GetDesktopFrame(QString fileName);
    QImage CopyDesktopImage(ID3D11Texture2D* texture);

private:
    ID3D11Device *m_pDevice;
    ID3D11DeviceContext *m_pDeviceContext;
    IDXGIOutputDuplication *m_pDuplication;

    int m_screenWidth;
    int m_screenHeight;
};

#endif // SCREENCAPTURE_H
