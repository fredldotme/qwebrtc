#include <memory>
#include <qwebrtcpeerconnectionfactory.hpp>
#include <webrtc/api/peerconnectioninterface.h>
#include "qwebrtcpeerconnection_p.hpp"
#include "qwebrtcpeerconnection.hpp"
#include "qwebrtcmediatrack_p.hpp"
#include "qwebrtcmediastream_p.hpp"
#include "qwebrtcicecandidate.hpp"
#include "qwebrtcconfiguration.hpp"
#include "qwebrtcdatachannel.hpp"
#include "qwebrtcdesktopvideosource_p.hpp"
//#include <webrtc/sdk/objc/Framework/Classes/videotoolboxvideocodecfactory.h>
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include <QThread>
#include <QCoreApplication>
#include <QDebug>

#include <libyuv/convert.h>

class QWebRTCPeerConnectionFactory_impl {
public:
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> native_interface;

    std::unique_ptr<rtc::Thread> m_workerThread;
    std::unique_ptr<rtc::Thread> m_signalingThread;
    std::unique_ptr<rtc::Thread> m_networkingThread;
};

//QWebRTCPeerConnectionFactory_impl::~QWebRTCPeerConnectionFactory_impl()
//{
//    m_workerThread->Quit();
//    m_signalingThread->Quit();
//    m_networkingThread->Quit();
//    while (m_workerThread->IsQuitting() || m_signalingThread->IsQuitting() || m_networkingThread->IsQuitting()) {
//        QThread::currentThread()->msleep(50);
//    }
//}

Q_DECLARE_METATYPE(QSharedPointer<QWebRTCIceCandidate>)
Q_DECLARE_METATYPE(QSharedPointer<QWebRTCMediaStream>)
Q_DECLARE_METATYPE(QSharedPointer<QWebRTCDataChannel>)


WebRTCFrameFetcher::WebRTCFrameFetcher() :
    QAbstractVideoFilter(),
    rtc::AdaptedVideoTrackSource()
{
}

QVideoFilterRunnable* WebRTCFrameFetcher::createFilterRunnable()
{
    return new FrameFetchRunnable(this);
}

void WebRTCFrameFetcher::OnFrameCaptured(const rtc::scoped_refptr<webrtc::I420Buffer>& buffer)
{
    if (!m_timestamper.isValid())
        m_timestamper.start();
    webrtc::VideoFrame frame(buffer, webrtc::VideoRotation::kVideoRotation_0, m_timestamper.nsecsElapsed() / 1000);
    OnFrame(frame);
}

int WebRTCFrameFetcher::AddRef() const {
    return rtc::AtomicOps::Increment(&ref_count_);
}

int WebRTCFrameFetcher::Release() const {
    const int count = rtc::AtomicOps::Decrement(&ref_count_);
    return count;
}

webrtc::MediaSourceInterface::SourceState WebRTCFrameFetcher::state() const {
    return webrtc::MediaSourceInterface::SourceState::kLive;
}

bool WebRTCFrameFetcher::remote() const {
    return false;
}

bool WebRTCFrameFetcher::is_screencast() const {
    return false;
}

rtc::Optional<bool> WebRTCFrameFetcher::needs_denoising() const {
    return rtc::Optional<bool>(false);
}

FrameFetchRunnable::FrameFetchRunnable(WebRTCFrameFetcher* filter) :
    QVideoFilterRunnable(),
    m_filter(filter)
{
}

QVideoFrame FrameFetchRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags)
{
    Q_UNUSED(surfaceFormat)
    Q_UNUSED(flags)

    if (!input)
    {
        return QVideoFrame();
    }

    if (!m_filter)
    {
        return *input;
    }

    input->map(QAbstractVideoBuffer::ReadOnly);
    if (input->mappedBytes() <= 0) {
        input->unmap();
        return *input;
    }

    rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(input->width(), input->height());
    libyuv::ABGRToI420(input->bits(), input->bytesPerLine(),
                       buffer->MutableDataY(), buffer->StrideY(),
                       buffer->MutableDataU(), buffer->StrideU(),
                       buffer->MutableDataV(), buffer->StrideV(),
                       buffer->width(), buffer->height());
    input->unmap();

    m_filter->OnFrameCaptured(buffer);

    return *input;
}

QWebRTCPeerConnectionFactory::QWebRTCPeerConnectionFactory()
{
    qRegisterMetaType<QSharedPointer<QWebRTCIceCandidate> >();
    qRegisterMetaType<QSharedPointer<QWebRTCMediaStream> >();
    qRegisterMetaType<QSharedPointer<QWebRTCDataChannel> >();

    m_impl = QSharedPointer<QWebRTCPeerConnectionFactory_impl>(new QWebRTCPeerConnectionFactory_impl());
    m_impl->m_networkingThread = rtc::Thread::CreateWithSocketServer();
    if (!m_impl->m_networkingThread->Start()) {
        qWarning() << "Faild to start networking thread";
    }


    m_impl->m_workerThread = rtc::Thread::Create();
    if (!m_impl->m_workerThread->Start()) {
        qWarning() <<"Failed to start worker thread.";
    }

    m_impl->m_signalingThread = rtc::Thread::Create();
    if (!m_impl->m_signalingThread->Start()) {
        qWarning() << "Failed to start signaling thread.";
    }

    //const auto encoder_factory = new webrtc::VideoToolboxVideoEncoderFactory();
    //const auto decoder_factory = new webrtc::VideoToolboxVideoDecoderFactory();

    m_impl->native_interface = webrtc::CreatePeerConnectionFactory(
                m_impl->m_networkingThread.get(), m_impl->m_workerThread.get(), m_impl->m_signalingThread.get(),
                nullptr, nullptr, nullptr/*encoder_factory, decoder_factory*/);
}

QSharedPointer<QWebRTCMediaTrack> QWebRTCPeerConnectionFactory::createAudioTrack(const QVariantMap& constraints, const QString& label)
{
    auto audioSource = m_impl->native_interface->CreateAudioSource(0);
    auto audioTrack = m_impl->native_interface->CreateAudioTrack(label.toStdString(), audioSource);
    return QSharedPointer<QWebRTCMediaTrack>(new QWebRTCMediaTrack_impl(audioTrack));
}

QSharedPointer<QWebRTCMediaTrack> QWebRTCPeerConnectionFactory::createVideoTrack(webrtc::VideoTrackSourceInterface* videoSource,
                                                                                 const QString& label)
{
    auto videoTrack = m_impl->native_interface->CreateVideoTrack(label.toStdString(), videoSource);
    return QSharedPointer<QWebRTCMediaTrack>(new QWebRTCMediaTrack_impl(videoTrack, videoSource));
}

QSharedPointer<QWebRTCMediaTrack> QWebRTCPeerConnectionFactory::createScreenCaptureTrack(const QString& label)
{
    auto videoSource = new rtc::RefCountedObject<QWebRTCDesktopVideoSource>();
    videoSource->moveToThread(QCoreApplication::instance()->thread());
    videoSource->Start();
    auto videoTrack = m_impl->native_interface->CreateVideoTrack(label.toStdString(), videoSource);
    return QSharedPointer<QWebRTCMediaTrack>(new QWebRTCMediaTrack_impl(videoTrack, videoSource));
}

QSharedPointer<QWebRTCMediaStream> QWebRTCPeerConnectionFactory::createMediaStream(const QString& label)
{
   return QSharedPointer<QWebRTCMediaStream>(new QWebRTCMediaStream_impl(m_impl->native_interface->CreateLocalMediaStream(label.toStdString())));
}

QSharedPointer<QWebRTCPeerConnection> QWebRTCPeerConnectionFactory::createPeerConnection(const QWebRTCConfiguration& config)
{
    rtc::LogMessage::LogToDebug(rtc::LS_WARNING);
    auto webRTCCOnfig = webrtc::PeerConnectionInterface::RTCConfiguration();
    std::vector<webrtc::PeerConnectionInterface::IceServer> servers;
    for (auto server : config.iceServers) {
        webrtc::PeerConnectionInterface::IceServer iceS;
        iceS.tls_cert_policy = server.tlsCertNoCheck ? webrtc::PeerConnectionInterface::kTlsCertPolicySecure : webrtc::PeerConnectionInterface::kTlsCertPolicyInsecureNoCheck;
        for (auto url : server.urls) {
            iceS.urls.push_back(url.toStdString());
        }
        iceS.username = server.username.toStdString();
        iceS.password = server.password.toStdString();
        servers.push_back(iceS);
    }
    webRTCCOnfig.servers = servers;
    auto conn = QSharedPointer<QWebRTCPeerConnection>(new QWebRTCPeerConnection());
    conn->m_impl->m_factory = m_impl;
    conn->m_impl->_conn = m_impl->native_interface->CreatePeerConnection(webRTCCOnfig,
            nullptr, nullptr, conn->m_impl);
    return conn;
}
