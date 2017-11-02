#ifndef CEF_LIBCEF_RENDERER_MEDIA_GPU_CEF_GPU_MEDIA_SERVICE_H_
# define CEF_LIBCEF_RENDERER_MEDIA_GPU_CEF_GPU_MEDIA_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"

#include "media/base/bitstream_buffer.h"

#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"

#include "gpu/ipc/service/gpu_channel_manager.h"

#include <map>

class CefMediaDelegate;
class CefGpuVideoCaptureManager;
class CefGpuMediaMsg_Initialize_Params;
class CefGpuMediaMsg_SetVideoPlan_Params;

class CefGpuMediaService {
  public:
    CefGpuMediaService(gpu::GpuChannelManager* channel_manager);
    ~CefGpuMediaService();

    void AddChannel(int client_id);
    void RemoveChannel(int client_id);
    void DestroyAllChannels();

    class ChannelListener :
      public IPC::Sender,
      public IPC::Listener,
      public gpu::GpuCommandBufferStub::DestructionObserver
    {
      public:
	ChannelListener(gpu::GpuChannel* channel,
			scoped_refptr<CefMediaDelegate> delegate);
	~ChannelListener();

	// IPC::Sender
	bool Send(IPC::Message* msg);

	// gpu::GpuCommandBufferStub::DestructionObserver
	void OnWillDestroyStub();

	// CefMediaDelegate callbacks
	void OnEndOfStream();
	void OnResolutionChanged(int width, int height);
	void OnVideoPTS(int64_t pts);
	void OnAudioPTS(int64_t pts);
	void OnHaveEnough();
	void OnFrameAvailable();

      private:

	// CefGpuVideoCaptureManager callback
	void OnFrameCaptured(
	  int32_t frame_id, gpu::Mailbox mailbox, gfx::Size coded_size,
	  gfx::Size natural_size, int64_t pts);

	// IPC::Listener and handlers
	bool OnMessageReceived(const IPC::Message& message);
	void OnInitialize(
	  CefGpuMediaMsg_Initialize_Params params,
	  IPC::Message* reply_message);
	void OnCleanup(IPC::Message* reply_message);
	void OnPlay();
	void OnPause();
	void OnResume();
	void OnStop();
	void OnFlush();
	void OnReset();
	void OnSetPlaybackRate(double rate);
	void OnSetVolume(float volume);
	void OnSetVideoPlan(CefGpuMediaMsg_SetVideoPlan_Params params);
	void OnRenderVideo(media::BitstreamBuffer buffer);
	void OnRenderAudio(media::BitstreamBuffer buffer);
	void OnReleaseFrame(int32_t frame_id);
	void OnUpdateSize(gfx::Size size);

	void DoCleanup();

	gpu::GpuChannel* channel_;
	gpu::GpuCommandBufferStub* stub_;
	scoped_refptr<CefMediaDelegate> delegate_;
	std::unique_ptr<CefGpuVideoCaptureManager> capture_manager_;

	bool audio_opened_;
	bool video_opened_;

	scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
	base::WeakPtrFactory<ChannelListener> weak_this_factory_;
    };

  private:

    gpu::GpuChannelManager* channel_manager_;
    std::map<int, std::unique_ptr<ChannelListener> > listeners_;
};

#endif /* !CEF_LIBCEF_RENDERER_MEDIA_GPU_CEF_GPU_MEDIA_SERVICE.H */
