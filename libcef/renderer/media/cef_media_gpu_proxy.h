#ifndef CEF_LIBCEF_RENDERER_MEDIA_CEF_MEDIA_GPU_PROXY_H_
# define CEF_LIBCEF_RENDERER_MEDIA_CEF_MEDIA_GPU_PROXY_H_

#include "base/macros.h"
#include "base/time/time.h"

#include "media/base/demuxer_stream.h"
#include "media/base/video_frame.h"
#include "media/base/decryptor.h"

#include "ipc/ipc_sender.h"
#include "ipc/ipc_listener.h"

#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/ipc/client/command_buffer_proxy_impl.h"

#include "include/cef_base.h"

#include "libcef/renderer/media/cef_stream_controller.h"
#include "libcef/renderer/media/gpu/cef_media_messages.h"

class CefMediaGpuProxy :
  public IPC::Sender,
  public IPC::Listener {

  public:

    class Client {
      public:
	enum Error {
	  kStreamError,
	  kRendererError,
	  kIPCError,
	};

	virtual void OnMediaGpuProxyError(Error error) = 0;
	virtual void OnFlushed() = 0;
	virtual void OnStatistics(media::DemuxerStream::Type type, int size) = 0;
	virtual void OnEndOfStream() = 0;
	virtual void OnResolutionChanged(int width, int height) = 0;
	virtual void OnHaveEnough() = 0;
	virtual void OnLastPTS(int64_t pts) = 0;
	virtual void OnPTSUpdate(int64_t pts) = 0;
	virtual void OnFrameCaptured(const scoped_refptr<media::VideoFrame>& frame) = 0;
	virtual void OnNeedKey() = 0;
    };

    CefMediaGpuProxy(Client *client,
		     const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
    ~CefMediaGpuProxy();

    static bool PlatformHasVP9Support();
    static bool PlatformHasOpusSupport();

    bool Send(IPC::Message *msg);

    bool InitializeRenderer(media::DemuxerStream* video_stream,
			    media::DemuxerStream* audio_stream,
			    content::ThreadSafeSender* thread_safe_sender,
			    bool& use_video_hole);
    void CleanupRenderer();

    void SetDecryptor(media::Decryptor* decryptor);

    bool HasAudio();
    bool HasVideo();

    void Start(base::TimeDelta start_time);
    void Play();
    void Pause();
    void Resume();
    void Stop();
    void Flush();
    void Reset();
    void SetPlaybackRate(double rate);
    void SetVolume(float volume);
    void SetVideoPlan(int x, int y, int width, int height, int display_width,
		      int display_height);

    // CefStreamController callbacks
    void SendAudioBuffer(media::BitstreamBuffer& buffer);
    void SendVideoBuffer(media::BitstreamBuffer& buffer);
    void OnLastAudioPTS(int64_t pts);
    void OnLastVideoPTS(int64_t pts);
    void OnStreamError();
    void OnNeedKey();
    base::SharedMemoryHandle OnShareToGpuProcess(base::SharedMemoryHandle handle);
    void VideoConfigurationChanged(gfx::Size size);

  private:

    // IPC Handlers
    bool OnMessageReceived(const IPC::Message& msg);
    void OnFlushed();
    void OnEndOfStream();
    void OnResolutionChanged(int width, int height);
    void OnHaveEnough();
    void OnAudioTimeUpdate(int64_t pts);
    void OnVideoTimeUpdate(int64_t pts);
    void OnFrameCaptured(CefGpuMediaMsg_Frame_Params params);
    void OnReleaseVideoBuffer(int32_t id);
    void OnReleaseAudioBuffer(int32_t id);

    // Video frame release callback
    void ReleaseMailbox(const uint32_t frame_id, const gpu::SyncToken& token);

    // Media tasks
    void UpdateAudioTimeTask(int64_t pts);
    void UpdateVideoTimeTask(int64_t pts);
    void ReleaseVideoBufferTask(int32_t id);
    void ReleaseAudioBufferTask(int32_t id);
    void OnLastAudioPTSTask(int64_t pts);
    void OnLastVideoPTSTask(int64_t pts);
    void OnFlushedTask();
    void OnEndOfStreamTask();
    void OnResolutionChangedTask(int width, int height);
    void OnHaveEnoughTask();
    void OnFrameCapturedTask(CefGpuMediaMsg_Frame_Params params);

    Client *client_;
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
    gpu::CommandBufferProxyImpl* command_buffer_proxy_;
    base::TimeDelta start_time_;

    std::unique_ptr<CefStreamController> video_stream_;
    std::unique_ptr<CefStreamController> audio_stream_;

    base::WeakPtrFactory<CefMediaGpuProxy> weak_ptr_factory_;
    base::WeakPtr<CefMediaGpuProxy> weak_ptr_;
};

#endif /* !CEF_LIBCEF_RENDERER_MEDIA_CEF_MEDIA_GPU_PROXY.H */
