#include "base/macros.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"

#include "gpu/ipc/service/gpu_channel.h"

#include "include/cef_app.h"
#include "include/cef_media_delegate.h"

#include "libcef/common/content_client.h"
#include "libcef/renderer/media/gpu/cef_gpu_media_service.h"
#include "libcef/renderer/media/gpu/cef_media_messages.h"
#include "libcef/renderer/media/gpu/cef_gpu_video_capture_manager.h"

// CefMediaDelegate callback class implementation
class CefMediaEventCallbackImpl : public CefMediaEventCallback {
  public:
    CefMediaEventCallbackImpl(
      base::WeakPtr<CefGpuMediaService::ChannelListener> listener,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
      : listener_(listener),
        task_runner_(task_runner) {
    }

    ~CefMediaEventCallbackImpl() {
    }

    void EndOfStream() override {
      LOG(INFO) << "Received end of stream";
      task_runner_->PostTask(FROM_HERE,
          base::Bind(&CefGpuMediaService::ChannelListener::OnEndOfStream, listener_));
    }

    void ResolutionChanged(int width, int height) override {
      LOG(INFO) << "Received resolution changed";
      task_runner_->PostTask(FROM_HERE,
          base::Bind(&CefGpuMediaService::ChannelListener::OnResolutionChanged, listener_,
            width, height));
    }

    void VideoPTS(int64_t pts) override {
      task_runner_->PostTask(FROM_HERE,
          base::Bind(&CefGpuMediaService::ChannelListener::OnVideoPTS, listener_, pts));
    }

    void AudioPTS(int64_t pts) override {
      task_runner_->PostTask(FROM_HERE,
          base::Bind(&CefGpuMediaService::ChannelListener::OnAudioPTS, listener_, pts));
    }

    void HaveEnough() override {
      LOG(INFO) << "Received have enough";
      task_runner_->PostTask(FROM_HERE,
          base::Bind(&CefGpuMediaService::ChannelListener::OnHaveEnough, listener_));
    }

    void FrameAvailable() override {
      task_runner_->PostTask(FROM_HERE,
          base::Bind(&CefGpuMediaService::ChannelListener::OnFrameAvailable, listener_));
    }

  private:

    base::WeakPtr<CefGpuMediaService::ChannelListener> listener_;
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

    IMPLEMENT_REFCOUNTING(CefMediaEventCallbackImpl);
};

// CefGpuMediaService implementation
CefGpuMediaService::CefGpuMediaService(gpu::GpuChannelManager* channel_manager)
  : channel_manager_(channel_manager) {
}

CefGpuMediaService::~CefGpuMediaService() {
  DestroyAllChannels();
}

void CefGpuMediaService::AddChannel(int client_id) {
  gpu::GpuChannel* channel = channel_manager_->LookupChannel(client_id);

  if (channel) {
    std::unique_ptr<ChannelListener> listener(
      new ChannelListener(
	channel, CefContentClient::Get()->application()->GetMediaDelegate()));

    listeners_[client_id] = std::move(listener);
  }
}

void CefGpuMediaService::RemoveChannel(int client_id) {
  std::map<int, std::unique_ptr<ChannelListener> >::iterator it =
    listeners_.find(client_id);

  if (it != listeners_.end())
    listeners_.erase(it);
}

void CefGpuMediaService::DestroyAllChannels() {
  listeners_.clear();
}

// CefGpuMediaService::ChannelListener implementation
CefGpuMediaService::ChannelListener::ChannelListener(
  gpu::GpuChannel* channel,
  scoped_refptr<CefMediaDelegate> delegate)
  : channel_(channel),
    stub_(NULL),
    delegate_(delegate),
    task_runner_(base::MessageLoop::current()->task_runner()),
    weak_this_factory_(this) {
  capture_manager_.reset(NULL);
  channel_->SetUnhandledMessageListener(this);
}

CefGpuMediaService::ChannelListener::~ChannelListener() {
  DoCleanup();
}

// IPC::Sender
bool CefGpuMediaService::ChannelListener::Send(IPC::Message* msg) {
  return channel_->Send(msg);
}

// gpu::GpuCommandBufferStub::DestructionObserver
void CefGpuMediaService::ChannelListener::OnWillDestroyStub() {
  DoCleanup();
}

// CefMediaDelegate callbacks
void CefGpuMediaService::ChannelListener::OnEndOfStream() {
  if (stub_) {
    channel_->Send(new CefGpuMediaMsg_EndOfStream(stub_->route_id()));
  }
}

void CefGpuMediaService::ChannelListener::OnResolutionChanged(int width, int height) {
  if (stub_) {
    channel_->Send(new CefGpuMediaMsg_ResolutionChanged(stub_->route_id(), width, height));
  }
}

void CefGpuMediaService::ChannelListener::OnVideoPTS(int64_t pts) {
  if (stub_) {
    channel_->Send(new CefGpuMediaMsg_VideoTimeUpdate(stub_->route_id(), pts));

    if (capture_manager_.get())
      capture_manager_->CurrentPTS(pts);
  }
}

void CefGpuMediaService::ChannelListener::OnAudioPTS(int64_t pts) {
  if (stub_)
    channel_->Send(new CefGpuMediaMsg_AudioTimeUpdate(stub_->route_id(), pts));
}

void CefGpuMediaService::ChannelListener::OnHaveEnough() {
  if (stub_) {
    channel_->Send(new CefGpuMediaMsg_HaveEnough(stub_->route_id()));
  }
}

void CefGpuMediaService::ChannelListener::OnFrameAvailable() {
  if (capture_manager_.get()) {
    capture_manager_->CaptureFrame(
      base::Bind(&CefGpuMediaService::ChannelListener::OnFrameCaptured,
		 weak_this_factory_.GetWeakPtr()));
  }
}

// CefGpuVideoCaptureManager callback
void CefGpuMediaService::ChannelListener::OnFrameCaptured(
  int32_t frame_id, gpu::Mailbox mailbox, gfx::Size coded_size,
  gfx::Size natural_size, int64_t pts)
{
  if (stub_) {
    gfx::Rect visible_rect(natural_size);
    CefGpuMediaMsg_Frame_Params params;

    params.coded_size = coded_size;
    params.natural_size = natural_size;
    params.visible_rect = visible_rect;
    params.frame_id = frame_id;
    params.mailbox = mailbox;
    params.pts = pts;

    channel_->Send(new CefGpuMediaMsg_FrameCaptured(stub_->route_id(), params));
  }
}

// IPC::Listener and handlers
bool CefGpuMediaService::ChannelListener::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;

  IPC_BEGIN_MESSAGE_MAP(CefGpuMediaService, message)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(CefGpuMediaMsg_HasVP9Support,
				    CefGpuMediaService::ChannelListener::OnHasVP9Support)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(CefGpuMediaMsg_HasOpusSupport,
				    CefGpuMediaService::ChannelListener::OnHasOpusSupport)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(CefGpuMediaMsg_Initialize,
				    CefGpuMediaService::ChannelListener::OnInitialize)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(CefGpuMediaMsg_Cleanup,
				    CefGpuMediaService::ChannelListener::OnCleanup)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(CefGpuMediaMsg_Reset,
				    CefGpuMediaService::ChannelListener::OnReset)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_Play,
			CefGpuMediaService::ChannelListener::OnPlay)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_Pause,
			CefGpuMediaService::ChannelListener::OnPause)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_Resume,
			CefGpuMediaService::ChannelListener::OnResume)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_Stop,
			CefGpuMediaService::ChannelListener::OnStop)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_Flush,
			CefGpuMediaService::ChannelListener::OnFlush)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_SetPlaybackRate,
			CefGpuMediaService::ChannelListener::OnSetPlaybackRate)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_SetVolume,
			CefGpuMediaService::ChannelListener::OnSetVolume)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_SetVideoPlan,
			CefGpuMediaService::ChannelListener::OnSetVideoPlan)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_RenderVideo,
			CefGpuMediaService::ChannelListener::OnRenderVideo)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_RenderAudio,
			CefGpuMediaService::ChannelListener::OnRenderAudio)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_ReleaseFrame,
			CefGpuMediaService::ChannelListener::OnReleaseFrame)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_UpdateSize,
			CefGpuMediaService::ChannelListener::OnUpdateSize)
    IPC_MESSAGE_UNHANDLED(handled = false)

  IPC_END_MESSAGE_MAP()

  return handled;
}

void CefGpuMediaService::ChannelListener::OnInitialize(
  CefGpuMediaMsg_Initialize_Params params,
  IPC::Message* reply_message) {
  CefRefPtr<CefMediaEventCallbackImpl> callbackImpl = new CefMediaEventCallbackImpl(
    weak_this_factory_.GetWeakPtr(), task_runner_);
  gpu::GpuCommandBufferStub* stub =
    channel_->LookupCommandBuffer(params.ipc_route_id);
  CefGpuMediaMsg_Initialize_Result result;

  DoCleanup();

  if (stub) {
    stub_ = stub;
    stub_->AddDestructionObserver(this);

    delegate_->SetEventCallback(callbackImpl);

    if (params.audio_codec == media::AudioCodec::kUnknownAudioCodec) {
      LOG(WARNING) << "No audio stream";
    } else if (!delegate_->CheckAudioCodec((cef_audio_codec_t)params.audio_codec)) {
      LOG(ERROR) << "Unsupported audio codec : " << media::GetCodecName(params.audio_codec);
    } else if (!delegate_->OpenAudio()) {
      LOG(ERROR) << "Failed to open audio device";
    } else {
      audio_opened_ = true;
    }

    if (params.video_codec == media::VideoCodec::kUnknownVideoCodec) {
      LOG(WARNING) << "No video stream";
    } else if (!delegate_->CheckVideoCodec((cef_video_codec_t)params.video_codec)) {
      LOG(ERROR) << "Unsupported video codec : " << media::GetCodecName(params.video_codec);
    } else if (!delegate_->OpenVideo()) {
      LOG(ERROR) << "Failed to open video device";
    } else {
      video_opened_ = true;
    }

    result.success = audio_opened_ || video_opened_;
    result.use_video_hole = true;
    result.max_audio_frame_count = delegate_->MaxAudioSampleCount();
    result.max_video_frame_count = delegate_->MaxVideoSampleCount();

    if (video_opened_ && delegate_->EnableVideoCapture()) {
      capture_manager_.reset(new CefGpuVideoCaptureManager(params.size, stub_, delegate_));
      result.use_video_hole = false;
    }
  } else {
     LOG(ERROR) << "No stub for route " << params.ipc_route_id;
     result.success = false;
  }

  CefGpuMediaMsg_Initialize::WriteReplyParams(reply_message, result);

  channel_->Send(reply_message);

  LOG(INFO) << "Media initialized on GPU side (success=" << (result.success ? "TRUE" : "FALSE") << ")";
}

void CefGpuMediaService::ChannelListener::OnCleanup(IPC::Message* reply_message) {
  DoCleanup();
  channel_->Send(reply_message);
}

void CefGpuMediaService::ChannelListener::OnReset(IPC::Message* reply_message) {
  delegate_->Reset();
  channel_->Send(reply_message);
}

void CefGpuMediaService::ChannelListener::OnPlay() {
  if (stub_) {
    delegate_->Play();
  }
}

void CefGpuMediaService::ChannelListener::OnPause() {
  if (stub_) {
    delegate_->Pause();
  }
}

void CefGpuMediaService::ChannelListener::OnResume() {
  if (stub_) {
    delegate_->Resume();
  }
}

void CefGpuMediaService::ChannelListener::OnStop() {
  if (stub_) {
    delegate_->Stop();
  }
}

void CefGpuMediaService::ChannelListener::OnFlush() {
  if (stub_) {
    delegate_->Flush();
    channel_->Send(new CefGpuMediaMsg_Flushed(stub_->route_id()));
  }
}

void CefGpuMediaService::ChannelListener::OnSetPlaybackRate(double rate) {
  if (stub_) {
    delegate_->SetSpeed(rate);
  }
}

void CefGpuMediaService::ChannelListener::OnSetVolume(float volume) {
  if (stub_) {
    delegate_->SetVolume(volume);
  }
}

void CefGpuMediaService::ChannelListener::OnSetVideoPlan(
  CefGpuMediaMsg_SetVideoPlan_Params params) {
  if (stub_) {
    delegate_->SetVideoPlan(
      params.x,
      params.y,
      params.width,
      params.height,
      params.display_width,
      params.display_height);
  }
}

void CefGpuMediaService::ChannelListener::OnRenderVideo(media::BitstreamBuffer buffer) {
  if (stub_) {
    std::unique_ptr<base::SharedMemory> shm(
      new base::SharedMemory(buffer.handle(), false));

    if (!shm->Map(buffer.size())) {
      LOG(WARNING) << "Failed to map video buffer";
      return;
    }

    delegate_->SendVideoBuffer((char*)shm->memory(), buffer.size(),
			       buffer.presentation_timestamp().InMilliseconds());

    shm->Unmap();

    channel_->Send(new CefGpuMediaMsg_ReleaseVideoBuffer(stub_->route_id(), buffer.id()));
  }
}

void CefGpuMediaService::ChannelListener::OnRenderAudio(media::BitstreamBuffer buffer) {
  if (stub_) {
    std::unique_ptr<base::SharedMemory> shm(
      new base::SharedMemory(buffer.handle(), false));

    if (!shm->Map(buffer.size())) {
      LOG(WARNING) << "Failed to map audio buffer";
      return;
    }

    delegate_->SendAudioBuffer((char*)shm->memory(), buffer.size(),
			       buffer.presentation_timestamp().InMilliseconds());

    shm->Unmap();

    channel_->Send(new CefGpuMediaMsg_ReleaseAudioBuffer(stub_->route_id(), buffer.id()));
  }
}

void CefGpuMediaService::ChannelListener::OnReleaseFrame(int32_t frame_id) {
  if (capture_manager_.get())
    capture_manager_->ReleaseFrame(frame_id);
}

void CefGpuMediaService::ChannelListener::OnUpdateSize(gfx::Size size) {
  if (capture_manager_.get())
    capture_manager_->UpdateSize(size);
}

void CefGpuMediaService::ChannelListener::DoCleanup() {
  if (audio_opened_)
    delegate_->CloseAudio();

  if (video_opened_)
    delegate_->CloseVideo();

  if (capture_manager_.get())
    capture_manager_.reset(NULL);

  delegate_->Cleanup();

  if (stub_)
    stub_ = NULL;
}

void CefGpuMediaService::ChannelListener::OnHasVP9Support(IPC::Message* reply_message) {
  bool support = delegate_->HasVP9Support();
  LOG(INFO) << "OnHasVP9Support: has_support=" << support;
  CefGpuMediaMsg_HasVP9Support::WriteReplyParams(reply_message, support);
  channel_->Send(reply_message);
}

void CefGpuMediaService::ChannelListener::OnHasOpusSupport(IPC::Message* reply_message) {
  bool support = delegate_->HasOpusSupport();
  LOG(INFO) << "OnHasOpusSupport: has_support=" << support;
  CefGpuMediaMsg_HasOpusSupport::WriteReplyParams(reply_message, support);
  channel_->Send(reply_message);
}
