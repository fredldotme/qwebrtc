#pragma once

#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <QAbstractVideoFilter>
#include <QVideoFilterRunnable>
#include <QElapsedTimer>

#include <webrtc/media/base/adaptedvideotracksource.h>
#include <webrtc/api/video/i420_buffer.h>
#include <webrtc/common_video/include/video_frame_buffer.h>

class QWebRTCPeerConnection;
class QWebRTCPeerConnectionFactory_impl;
class QWebRTCMediaTrack;
class QWebRTCMediaStream;
class QWebRTCConfiguration;

class WebRTCFrameFetcher : public QAbstractVideoFilter,
        public rtc::AdaptedVideoTrackSource {
    Q_OBJECT

public:
    WebRTCFrameFetcher();
    ~WebRTCFrameFetcher() = default;

    QVideoFilterRunnable* createFilterRunnable() Q_DECL_OVERRIDE;

    void OnFrameCaptured(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer);

    virtual int AddRef() const override;
    virtual int Release() const override;

    virtual webrtc::MediaSourceInterface::SourceState state() const override;
    virtual bool remote() const override;
    virtual bool is_screencast() const override;
    virtual rtc::Optional<bool> needs_denoising() const override;


private:
    mutable volatile int ref_count_;
    QElapsedTimer m_timestamper;
};

class FrameFetchRunnable : public QVideoFilterRunnable
{
public:
    FrameFetchRunnable(WebRTCFrameFetcher* filter);
    QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags) Q_DECL_OVERRIDE;

private:
    WebRTCFrameFetcher* m_filter;
};

class QWebRTCPeerConnectionFactory : public QObject {
public:
    QWebRTCPeerConnectionFactory();

    QSharedPointer<QWebRTCMediaTrack> createAudioTrack(const QVariantMap& constraints, const QString& trackId = QString());

    QSharedPointer<QWebRTCMediaTrack> createVideoTrack(webrtc::VideoTrackSourceInterface* videoSource,
                                                       const QString& trackId = QString());

    QSharedPointer<QWebRTCMediaTrack> createScreenCaptureTrack(const QString& trackId = QString());

    QSharedPointer<QWebRTCMediaStream> createMediaStream(const QString& label);

    QSharedPointer<QWebRTCPeerConnection> createPeerConnection(const QWebRTCConfiguration&);

private:
    // This pointer is shared among all peer connections to ensure that all resources allocated by the
    // factory are not deallocated (e.g. the webrtc threads)
    QSharedPointer<QWebRTCPeerConnectionFactory_impl> m_impl;
};
