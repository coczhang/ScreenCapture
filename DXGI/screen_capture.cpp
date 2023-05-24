#include "screen_capture.h"

#include <QDebug>

ScreenCapture::ScreenCapture(QObject *parent) : QObject(parent)
    ,m_pDevice(nullptr),m_pDeviceContext(nullptr),m_pDuplication(nullptr)
{

}

ScreenCapture::~ScreenCapture()
{
    if (m_pDuplication) {
        m_pDuplication->Release();
    }

    if (m_pDevice) {
        m_pDevice->Release();
    }

    if (m_pDeviceContext) {
        m_pDeviceContext->Release();
    }
}

bool ScreenCapture::InitD3D11Device()
{
    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = std::extent<decltype(driverTypes)>::value;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT numFeatureLevels = std::extent<decltype(featureLevels)>::value;
    D3D_FEATURE_LEVEL featureLevel;

    for (UINT i = 0; i < numDriverTypes; ++i)
    {
        HRESULT hr = D3D11CreateDevice(nullptr, driverTypes[i], nullptr, 0, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &m_pDevice, &featureLevel, &m_pDeviceContext);
        if (SUCCEEDED(hr)) {
            break;
        }
    }

    if (m_pDevice == nullptr || m_pDeviceContext == nullptr) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "InitD3D11Device Failed";
        return false;
    }

    return true;

}

bool ScreenCapture::InitDuplication()
{
    HRESULT hr = S_OK;

    IDXGIDevice* dxgiDevice = nullptr;
    hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    if (FAILED(hr)) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "QueryInterface Failed:" << hr;
        return false;
    }

    if (dxgiDevice == nullptr) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "dxgiDevice is  nullptr";
        return false;
    }

    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiDevice->Release();
    if (FAILED(hr)) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "GetAdapter Failed:" << hr;
        return false;
    }

    if (dxgiAdapter == nullptr) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "dxgiAdapter is  nullptr";
        return false;
    }

    UINT output = 0;
    IDXGIOutput* dxgiOutput = nullptr;
    while (true)
    {
        hr = dxgiAdapter->EnumOutputs(output++, &dxgiOutput);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            return false;
        } else {
            if (dxgiOutput == nullptr) {
                qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "dxgiOutput is  nullptr";
                break;
            }

            DXGI_OUTPUT_DESC desc;
            dxgiOutput->GetDesc(&desc);
            m_screenWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
            m_screenHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
            break;
        }
    }
    dxgiAdapter->Release();

    IDXGIOutput1* dxgiOutput1 = nullptr;
    hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&dxgiOutput1));
    dxgiOutput->Release();
    if (FAILED(hr)) {
        return false;
    }

    if (dxgiOutput1 == nullptr) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "dxgiOutput1 is  nullptr";
        return false;
    }

    hr = dxgiOutput1->DuplicateOutput(m_pDevice, &m_pDuplication);
    dxgiOutput1->Release();
    if (FAILED(hr)) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "DuplicateOutput Failed";
        return false;
    }

    return true;
}

bool ScreenCapture::GetDesktopFrame(QString fileName)
{
    HRESULT hr = S_OK;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* resource = nullptr;
    ID3D11Texture2D* acquireFrame = nullptr;
    ID3D11Texture2D* texture = nullptr;

    if (m_pDuplication == nullptr) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "m_pDuplication is  nullptr";
        return false;
    }

    hr = m_pDuplication->AcquireNextFrame(0, &frameInfo, &resource);
    if (FAILED(hr)) {
        return false;
    }

    if (resource == nullptr) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "resource is  nullptr";
        return false;
    }

    hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&acquireFrame));
    resource->Release();
    if (FAILED(hr)) {
        return false;
    }

    if (acquireFrame == nullptr) {
        qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "acquireFrame is  nullptr";
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    acquireFrame->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    m_pDevice->CreateTexture2D(&desc, NULL, &texture);
    if (texture) {
        m_pDeviceContext->CopyResource(texture, acquireFrame);
        QImage image = this->CopyDesktopImage(texture);
        image.save(fileName, "PNG");
    }
    acquireFrame->Release();

    hr = m_pDuplication->ReleaseFrame();
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

QImage ScreenCapture::CopyDesktopImage(ID3D11Texture2D* texture)
{
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    m_pDeviceContext->Map(texture, 0, D3D11_MAP_READ, 0, &mapped_resource);

    QImage image(static_cast<uchar *>(mapped_resource.pData), m_screenWidth, m_screenHeight, QImage::Format_ARGB32);

    texture->Release();
    texture = nullptr;

    return image;
}
