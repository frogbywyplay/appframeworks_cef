#include "base/bind.h"
#include "base/logging.h"

#include "media/base/video_types.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/video_decoder_config.h"

#include "content/renderer/render_thread_impl.h"
#include "content/common/gpu/client/context_provider_command_buffer.h"

#include "gpu/command_buffer/common/mailbox_holder.h"
#include "gpu/command_buffer/client/gles2_lib.h"

#include "libcef/renderer/media/cef_media_gpu_proxy.h"

namespace {

  class VideoStreamControllerClient :
      public CefStreamController::Client,
      public base::RefCountedThreadSafe<VideoStreamControllerClient> {
    public :

      VideoStreamControllerClient(scoped_refptr<CefMediaGpuProxy> proxy)
	: proxy_(proxy) {
      }

      virtual void OnLastPTS(int64_t pts) override {
	proxy_->OnLastVideoPTS(pts);
      }

      virtual void OnNeedKey() override {
	proxy_->OnNeedKey();
      }

      virtual void OnError() override {
	proxy_->OnStreamError();
      }

      virtual base::SharedMemoryHandle ShareBufferToGpuProcess(
	base::SharedMemoryHandle handle) override {
	return proxy_->OnShareToGpuProcess(handle);
      }

      virtual void SendBuffer(media::BitstreamBuffer& buffer) override {
	proxy_->SendVideoBuffer(buffer);
      }

      virtual void VideoConfigurationChanged(gfx::Size size) override {
	proxy_->VideoConfigurationChanged(size);
      }

      virtual void AddDecodedBytes(size_t count) override {
	proxy_->AddVideoDecodedBytes(count);
      }

      virtual void AddDecodedFrames(size_t count) override {
	proxy_->AddVideoDecodedFrames(count);
      }

    private:
      friend class base::RefCountedThreadSafe<VideoStreamControllerClient>;
      virtual ~VideoStreamControllerClient() override {
      }

      scoped_refptr<CefMediaGpuProxy> proxy_;

  };

  class AudioStreamControllerClient :
      public CefStreamController::Client,
      public base::RefCountedThreadSafe<AudioStreamControllerClient> {
    public :

      AudioStreamControllerClient(scoped_refptr<CefMediaGpuProxy> proxy)
	: proxy_(proxy) {
      }

      virtual void OnLastPTS(int64_t pts) override {
	proxy_->OnLastAudioPTS(pts);
      }

      virtual void OnNeedKey() override {
	proxy_->OnNeedKey();
      }

      virtual void OnError() override {
	proxy_->OnStreamError();
      }

      virtual base::SharedMemoryHandle ShareBufferToGpuProcess(
	base::SharedMemoryHandle handle) override {
	return proxy_->OnShareToGpuProcess(handle);
      }

      virtual void SendBuffer(media::BitstreamBuffer& buffer) override {
	proxy_->SendAudioBuffer(buffer);
      }

      virtual void AddDecodedBytes(size_t count) override {
	proxy_->AddVideoDecodedBytes(count);
      }

    private:
      friend class base::RefCountedThreadSafe<AudioStreamControllerClient>;
      virtual ~AudioStreamControllerClient() override {
      }

      scoped_refptr<CefMediaGpuProxy> proxy_;

  };

}

CefMediaGpuProxy::CefMediaGpuProxy(CefMediaGpuProxy::Client *client,
				   const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
  : client_(client),
    task_runner_(task_runner),
    command_buffer_proxy_(
      content::RenderThreadImpl::current()
        ->SharedMainThreadContextProvider()->GetCommandBufferProxy()),
    initialized_(false),
    weak_factory_(this) {
  video_stream_ = NULL;
  video_stream_ = NULL;
}

CefMediaGpuProxy::~CefMediaGpuProxy() {
  if (initialized_) {
    command_buffer_proxy_->channel()->RemoveRoute(command_buffer_proxy_->route_id());
  }
}

bool CefMediaGpuProxy::Send(IPC::Message *msg) {
  if (!command_buffer_proxy_->channel()->Send(msg)) {
    if (client_)
      client_->OnMediaGpuProxyError(CefMediaGpuProxy::Client::kIPCError);
    return false;
  }
  return true;
}

bool CefMediaGpuProxy::InitializeRenderer(media::DemuxerStream* video_stream,
					  media::DemuxerStream* audio_stream,
					  content::ThreadSafeSender* thread_safe_sender,
					  bool& use_video_hole) {
  CefGpuMediaMsg_Initialize_Params params;
  CefGpuMediaMsg_Initialize_Result result;

  params.ipc_route_id = command_buffer_proxy_->route_id();

  if (audio_stream)
    params.audio_codec = audio_stream->audio_decoder_config().codec();
  else
    params.audio_codec = media::AudioCodec::kUnknownAudioCodec;

  if (video_stream) {
    params.video_codec = video_stream->video_decoder_config().codec();
    params.size = video_stream->video_decoder_config().coded_size();
  } else {
    params.video_codec = media::VideoCodec::kUnknownVideoCodec;
    params.size = gfx::Size();
  }

  if (!command_buffer_proxy_->channel()->Send(
	new CefGpuMediaMsg_Initialize(
	  command_buffer_proxy_->route_id(), params, &result))) {
    LOG(ERROR) << "Renderer initialization failed on GPU side";
    return false;
  }

  if (result.success) {
    if (video_stream) {
      video_stream_ =
	new CefStreamController(
	  video_stream, result.max_video_frame_count, thread_safe_sender,
	  new VideoStreamControllerClient(scoped_refptr<CefMediaGpuProxy>(this)));
    }

    if (audio_stream) {
      audio_stream_ =
	new CefStreamController(
	  audio_stream, result.max_audio_frame_count, thread_safe_sender,
	  new AudioStreamControllerClient(scoped_refptr<CefMediaGpuProxy>(this)));
    }

    command_buffer_proxy_->channel()->AddRoute(command_buffer_proxy_->route_id(), weak_factory_.GetWeakPtr());
    use_video_hole = result.use_video_hole;
  }

  initialized_ = result.success;

  return result.success;
}

void CefMediaGpuProxy::CleanupRenderer() {
  Send(new CefGpuMediaMsg_Cleanup(command_buffer_proxy_->route_id()));

  if (audio_stream_.get())
    audio_stream_ = NULL;

  if (video_stream_.get())
    video_stream_ = NULL;

  client_ = NULL;
}

void CefMediaGpuProxy::SetDecryptor(media::Decryptor* decryptor) {
  if (audio_stream_.get())
    audio_stream_->SetDecryptor(decryptor);

  if (video_stream_.get())
    video_stream_->SetDecryptor(decryptor);
}

bool CefMediaGpuProxy::HasAudio() {
  return video_stream_.get() != NULL;
}

bool CefMediaGpuProxy::HasVideo() {
  return audio_stream_.get() != NULL;
}

void CefMediaGpuProxy::Start(base::TimeDelta start_time) {
  start_time_ = start_time;

  if (video_stream_.get())
    video_stream_->SetStartTime(start_time);

  if (audio_stream_.get())
    audio_stream_->SetStartTime(start_time);
}

void CefMediaGpuProxy::Feed() {
  if (video_stream_.get())
    video_stream_->Feed();

  if (audio_stream_.get())
    audio_stream_->Feed();
}

void CefMediaGpuProxy::Play() {
  Send(new CefGpuMediaMsg_Play(command_buffer_proxy_->route_id()));
}

void CefMediaGpuProxy::Pause() {
  Send(new CefGpuMediaMsg_Pause(command_buffer_proxy_->route_id()));
}

void CefMediaGpuProxy::Resume() {
  Send(new CefGpuMediaMsg_Resume(command_buffer_proxy_->route_id()));
}

void CefMediaGpuProxy::Stop() {
  Send(new CefGpuMediaMsg_Stop(command_buffer_proxy_->route_id()));

  if (video_stream_.get())
    video_stream_->Stop();

  if (audio_stream_.get())
    audio_stream_->Stop();
}

void CefMediaGpuProxy::Flush() {
  Send(new CefGpuMediaMsg_Flush(command_buffer_proxy_->route_id()));

  if (video_stream_.get())
    video_stream_->Abort();

  if (audio_stream_.get())
    audio_stream_->Abort();
}

void CefMediaGpuProxy::Reset() {
  Send(new CefGpuMediaMsg_Reset(command_buffer_proxy_->route_id()));

  if (video_stream_.get())
    video_stream_->Abort();

  if (audio_stream_.get())
    audio_stream_->Abort();
}

void CefMediaGpuProxy::SetPlaybackRate(double rate) {
  Send(new CefGpuMediaMsg_SetPlaybackRate(command_buffer_proxy_->route_id(), rate));
}

void CefMediaGpuProxy::SetVolume(float volume) {
  Send(new CefGpuMediaMsg_SetVolume(command_buffer_proxy_->route_id(), volume));
}

void CefMediaGpuProxy::SetVideoPlan(int x, int y, int width, int height,
					int display_width, int display_height)
{
  CefGpuMediaMsg_SetVideoPlan_Params params;

  params.x = x;
  params.y = y;
  params.width = width;
  params.height = height;
  params.display_width = display_width;
  params.display_height = display_height;

  Send(new CefGpuMediaMsg_SetVideoPlan(command_buffer_proxy_->route_id(), params));
}

// CefStreamController callbacks
void CefMediaGpuProxy::SendAudioBuffer(media::BitstreamBuffer& buffer) {
  Send(new CefGpuMediaMsg_RenderAudio(command_buffer_proxy_->route_id(), buffer));
}

void CefMediaGpuProxy::SendVideoBuffer(media::BitstreamBuffer& buffer) {
  Send(new CefGpuMediaMsg_RenderVideo(command_buffer_proxy_->route_id(), buffer));
}

void CefMediaGpuProxy::OnLastAudioPTS(int64_t pts) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnLastAudioPTSTask, this, pts));
}

void CefMediaGpuProxy::OnLastVideoPTS(int64_t pts) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnLastVideoPTSTask, this, pts));
}

void CefMediaGpuProxy::OnStreamError() {
  LOG(ERROR) << "Error in stream controller";
  if (client_)
    client_->OnMediaGpuProxyError(CefMediaGpuProxy::Client::kStreamError);
}

void CefMediaGpuProxy::OnNeedKey() {
  if (client_)
    client_->OnNeedKey();
}

base::SharedMemoryHandle CefMediaGpuProxy::OnShareToGpuProcess(
  base::SharedMemoryHandle handle) {
  return command_buffer_proxy_->channel()->ShareToGpuProcess(handle);
}

void CefMediaGpuProxy::VideoConfigurationChanged(gfx::Size new_size) {
  Send(new CefGpuMediaMsg_UpdateSize(command_buffer_proxy_->route_id(), new_size));
}

void CefMediaGpuProxy::AddVideoDecodedBytes(size_t count) {
  if (client_)
    client_->AddVideoDecodedBytes(count);
}

void CefMediaGpuProxy::AddVideoDecodedFrames(size_t count) {
  if (client_)
    client_->AddVideoDecodedFrames(count);
}

void CefMediaGpuProxy::AddAudioDecodedBytes(size_t count) {
  if (client_)
    client_->AddAudioDecodedBytes(count);
}

// IPC Handlers
bool CefMediaGpuProxy::OnMessageReceived(const IPC::Message& msg)
{
  bool handled = true;

  IPC_BEGIN_MESSAGE_MAP(CefMediaGpuProxy, msg)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_ReleaseVideoBuffer,
                        CefMediaGpuProxy::OnReleaseVideoBuffer)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_ReleaseAudioBuffer,
                        CefMediaGpuProxy::OnReleaseAudioBuffer)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_Flushed,
                        CefMediaGpuProxy::OnFlushed)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_EndOfStream,
                        CefMediaGpuProxy::OnEndOfStream)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_ResolutionChanged,
                        CefMediaGpuProxy::OnResolutionChanged)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_HaveEnough,
                        CefMediaGpuProxy::OnHaveEnough)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_AudioTimeUpdate,
                        CefMediaGpuProxy::OnAudioTimeUpdate)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_VideoTimeUpdate,
                        CefMediaGpuProxy::OnVideoTimeUpdate)
    IPC_MESSAGE_HANDLER(CefGpuMediaMsg_FrameCaptured,
                        CefMediaGpuProxy::OnFrameCaptured)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void CefMediaGpuProxy::OnFlushed() {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnFlushedTask, this));
}

void CefMediaGpuProxy::OnEndOfStream() {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnEndOfStreamTask, this));
}

void CefMediaGpuProxy::OnResolutionChanged(int width, int height) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnResolutionChangedTask, this, width, height));
}

void CefMediaGpuProxy::OnHaveEnough() {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnHaveEnoughTask, this));
}

void CefMediaGpuProxy::OnAudioTimeUpdate(int64_t pts) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::UpdateAudioTimeTask, this, pts));
}

void CefMediaGpuProxy::OnVideoTimeUpdate(int64_t pts) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::UpdateVideoTimeTask, this, pts));
}

void CefMediaGpuProxy::OnFrameCaptured(CefGpuMediaMsg_Frame_Params params) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnFrameCapturedTask, this, params));
}

void CefMediaGpuProxy::OnReleaseVideoBuffer(int32_t id) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::ReleaseVideoBufferTask, this, id));
}

void CefMediaGpuProxy::OnReleaseAudioBuffer(int32_t id) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::ReleaseAudioBufferTask, this, id));
}

// Video frame release callback
void CefMediaGpuProxy::ReleaseMailbox(const uint32_t frame_id, const gpu::SyncToken& token) {
  Send(new CefGpuMediaMsg_ReleaseFrame(command_buffer_proxy_->route_id(), frame_id));
}

// Media tasks
void CefMediaGpuProxy::UpdateAudioTimeTask(int64_t pts) {
  if (client_)
    client_->OnPTSUpdate(pts);

  if (audio_stream_.get())
    audio_stream_->OnPTSUpdate(pts);
}

void CefMediaGpuProxy::UpdateVideoTimeTask(int64_t pts) {
  // Audio PTS is the reference, use video PTS only when we have no audio
  if (!audio_stream_.get() && client_)
    client_->OnPTSUpdate(pts);

  if (video_stream_.get())
    video_stream_->OnPTSUpdate(pts);
}

void CefMediaGpuProxy::ReleaseVideoBufferTask(int32_t id) {
  if (video_stream_.get())
    video_stream_->ReleaseBuffer(id);
}

void CefMediaGpuProxy::ReleaseAudioBufferTask(int32_t id) {
  if (audio_stream_.get())
    audio_stream_->ReleaseBuffer(id);
}

void CefMediaGpuProxy::OnLastAudioPTSTask(int64_t pts) {
  if (!video_stream_.get() && client_)
    client_->OnLastPTS(pts);
}

void CefMediaGpuProxy::OnLastVideoPTSTask(int64_t pts) {
  if (client_)
    client_->OnLastPTS(pts);
}

void CefMediaGpuProxy::OnFlushedTask() {
  if (client_)
    client_->OnFlushed();
}

void CefMediaGpuProxy::OnEndOfStreamTask() {
  if (client_)
    client_->OnEndOfStream();
}

void CefMediaGpuProxy::OnResolutionChangedTask(int width, int height) {
  if (client_)
    client_->OnResolutionChanged(width, height);
}

void CefMediaGpuProxy::OnHaveEnoughTask() {
  if (client_)
    client_->OnHaveEnough();
}

void CefMediaGpuProxy::OnFrameCapturedTask(CefGpuMediaMsg_Frame_Params params) {
  if (params.mailbox.IsZero()) {
    if (client_)
      client_->OnMediaGpuProxyError(CefMediaGpuProxy::Client::kRendererError);
    return;
  }

  gpu::MailboxHolder mailbox_holder[media::VideoFrame::kMaxPlanes] = {
    gpu::MailboxHolder(params.mailbox, gpu::SyncToken(), GL_TEXTURE_2D)
  };

  if (client_)
    client_->OnFrameCaptured(
      media::VideoFrame::WrapNativeTextures(
	media::VideoPixelFormat::PIXEL_FORMAT_ARGB, mailbox_holder,
	base::Bind(&CefMediaGpuProxy::ReleaseMailbox, this, params.frame_id),
	params.coded_size, params.visible_rect, params.natural_size,
	base::TimeDelta::FromMilliseconds(params.pts) + start_time_));
}

bool CefMediaGpuProxy::PlatformHasVP9Support() {
  bool support;
  gpu::CommandBufferProxyImpl* command_buffer_proxy =
    content::RenderThreadImpl::current()
      ->SharedMainThreadContextProvider()->GetCommandBufferProxy();

  if (command_buffer_proxy) {
    if (!command_buffer_proxy->channel()->Send(
	  new CefGpuMediaMsg_HasVP9Support(command_buffer_proxy->route_id(), &support))) {
      LOG(WARNING) << "CefGpuMediaMsg_HasVP9Support IPC failed";
      return false;
    }

    return support;
  }

  LOG(ERROR) << "No command_buffer_proxy";
  return false;
}

bool CefMediaGpuProxy::PlatformHasOpusSupport() {
  bool support;
  gpu::CommandBufferProxyImpl* command_buffer_proxy =
    content::RenderThreadImpl::current()
      ->SharedMainThreadContextProvider()->GetCommandBufferProxy();

  if (command_buffer_proxy) {
    if (!command_buffer_proxy->channel()->Send(
	  new CefGpuMediaMsg_HasOpusSupport(command_buffer_proxy->route_id(), &support))) {
      LOG(WARNING) << "CefGpuMediaMsg_HasOpusSupport IPC failed";
      return false;
    }

    return support;
  }

  LOG(ERROR) << "No command_buffer_proxy";
  return false;
}