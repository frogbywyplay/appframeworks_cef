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
      public CefStreamController::Client {
    public :

      VideoStreamControllerClient(CefMediaGpuProxy *proxy)
	: proxy_(proxy) {
      }

      ~VideoStreamControllerClient() {
      }

      void OnLastPTS(int64_t pts) {
	proxy_->OnLastVideoPTS(pts);
      }

      void OnNeedKey() {
	proxy_->OnNeedKey();
      }

      void OnError() {
	proxy_->OnStreamError();
      }

      base::SharedMemoryHandle ShareBufferToGpuProcess(
	base::SharedMemoryHandle handle) {
	return proxy_->OnShareToGpuProcess(handle);
      }

      void SendBuffer(media::BitstreamBuffer& buffer) {
	proxy_->SendVideoBuffer(buffer);
      }

      void VideoConfigurationChanged(gfx::Size size) {
	proxy_->VideoConfigurationChanged(size);
      }

    private:
      CefMediaGpuProxy *proxy_;

  };

  class AudioStreamControllerClient :
      public CefStreamController::Client {
    public :

      AudioStreamControllerClient(CefMediaGpuProxy *proxy)
	: proxy_(proxy) {
      }

      ~AudioStreamControllerClient() {
      }

      void OnLastPTS(int64_t pts) {
	proxy_->OnLastAudioPTS(pts);
      }

      void OnNeedKey() {
	proxy_->OnNeedKey();
      }

      void OnError() {
	proxy_->OnStreamError();
      }

      base::SharedMemoryHandle ShareBufferToGpuProcess(
	base::SharedMemoryHandle handle) {
	return proxy_->OnShareToGpuProcess(handle);
      }

      void SendBuffer(media::BitstreamBuffer& buffer) {
	proxy_->SendAudioBuffer(buffer);
      }

      void VideoConfigurationChanged(gfx::Size size) {
      }

    private:
      CefMediaGpuProxy *proxy_;

  };

}

CefMediaGpuProxy::CefMediaGpuProxy(CefMediaGpuProxy::Client *client,
				   const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
  : client_(client),
    task_runner_(task_runner),
    command_buffer_proxy_(
      content::RenderThreadImpl::current()
        ->SharedMainThreadContextProvider()->GetCommandBufferProxy()),
    weak_ptr_factory_(this),
    weak_ptr_(weak_ptr_factory_.GetWeakPtr()) {
  video_stream_.reset(NULL);
  video_stream_.reset(NULL);
}

CefMediaGpuProxy::~CefMediaGpuProxy() {
}

bool CefMediaGpuProxy::Send(IPC::Message *msg) {
  if (!command_buffer_proxy_->channel()->Send(msg)) {
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

  LOG(INFO) << "Initiazing renderer on host side";

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
    LOG(ERROR) << "Renderer initialization failed onf GPU side";
    return false;
  }

  if (result.success) {
    if (video_stream) {
      video_stream_.reset(
	new CefStreamController(
	  video_stream, result.max_video_frame_count, thread_safe_sender,
	  new VideoStreamControllerClient(this)));
    }

    if (audio_stream) {
      audio_stream_.reset(
	new CefStreamController(
	  audio_stream, result.max_audio_frame_count, thread_safe_sender,
	  new AudioStreamControllerClient(this)));
    }

    command_buffer_proxy_->channel()->AddRoute(command_buffer_proxy_->route_id(), weak_ptr_);
    use_video_hole = result.use_video_hole;
  }

  LOG(INFO) << "Renderer initialized :"
	    << " success=" << (result.success ? "TRUE" : "FALSE")
	    << " use_video_hole=" << (use_video_hole ? "TRUE" : "FALSE");

  return result.success;
}

void CefMediaGpuProxy::CleanupRenderer() {
  LOG(INFO) << "Cleaning renderer on host side";

  Send(new CefGpuMediaMsg_Cleanup(command_buffer_proxy_->route_id()));

  if (audio_stream_.get())
    audio_stream_.reset(NULL);

  if (video_stream_.get())
    video_stream_.reset(NULL);
}

void CefMediaGpuProxy::SetDecryptor(media::Decryptor* decryptor) {
  LOG(INFO) << "Receiving decryptor";

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
  LOG(INFO) << "Start renderer (start_time=" << start_time.InMilliseconds() << ")";

  start_time_ = start_time;

  if (video_stream_.get())
    video_stream_->SetStartTime(start_time);

  if (audio_stream_.get())
    audio_stream_->SetStartTime(start_time);
}


void CefMediaGpuProxy::Play() {
  LOG(INFO) << "Play";

  Send(new CefGpuMediaMsg_Play(command_buffer_proxy_->route_id()));

  if (video_stream_.get())
    video_stream_->Feed();

  if (audio_stream_.get())
    audio_stream_->Feed();
}

void CefMediaGpuProxy::Pause() {
  LOG(INFO) << "Pause";
  Send(new CefGpuMediaMsg_Pause(command_buffer_proxy_->route_id()));
}

void CefMediaGpuProxy::Resume() {
  LOG(INFO) << "Resume";
  Send(new CefGpuMediaMsg_Resume(command_buffer_proxy_->route_id()));
}

void CefMediaGpuProxy::Stop() {
  LOG(INFO) << "Stop";

  Send(new CefGpuMediaMsg_Stop(command_buffer_proxy_->route_id()));

  if (video_stream_.get())
    video_stream_->Stop();

  if (audio_stream_.get())
    audio_stream_->Stop();
}

void CefMediaGpuProxy::Flush() {
  LOG(INFO) << "Flush";

  Send(new CefGpuMediaMsg_Flush(command_buffer_proxy_->route_id()));

  if (video_stream_.get())
    video_stream_->Abort();

  if (audio_stream_.get())
    audio_stream_->Abort();
}

void CefMediaGpuProxy::Reset() {
  LOG(INFO) << "Reset";

  Send(new CefGpuMediaMsg_Reset(command_buffer_proxy_->route_id()));

  if (video_stream_.get())
    video_stream_->Abort();

  if (audio_stream_.get())
    audio_stream_->Abort();
}

void CefMediaGpuProxy::SetPlaybackRate(double rate) {
  LOG(INFO) << "Set playback rate " << rate;
  Send(new CefGpuMediaMsg_SetPlaybackRate(command_buffer_proxy_->route_id(), rate));
}

void CefMediaGpuProxy::SetVolume(float volume) {
  LOG(INFO) << "Set volume " << volume;
  Send(new CefGpuMediaMsg_SetVolume(command_buffer_proxy_->route_id(), volume));
}

void CefMediaGpuProxy::SetVideoPlan(int x, int y, int width, int height,
					int display_width, int display_height)
{
  CefGpuMediaMsg_SetVideoPlan_Params params;

  LOG(INFO) << "Set video plan x=" << x
	    << " y=" << y
	    << " width=" << width
	    << " height=" << height
	    << " display_width=" << display_width
	    << " display_height=" << display_height;

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
    base::Bind(&CefMediaGpuProxy::OnLastAudioPTSTask, weak_ptr_, pts));
}

void CefMediaGpuProxy::OnLastVideoPTS(int64_t pts) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnLastVideoPTSTask, weak_ptr_, pts));
}

void CefMediaGpuProxy::OnStreamError() {
  LOG(ERROR) << "Error in stream controller";
  client_->OnMediaGpuProxyError(CefMediaGpuProxy::Client::kStreamError);
}

void CefMediaGpuProxy::OnNeedKey() {
  client_->OnNeedKey();
}

base::SharedMemoryHandle CefMediaGpuProxy::OnShareToGpuProcess(
  base::SharedMemoryHandle handle) {
  return command_buffer_proxy_->channel()->ShareToGpuProcess(handle);
}

void CefMediaGpuProxy::VideoConfigurationChanged(gfx::Size new_size) {
  Send(new CefGpuMediaMsg_UpdateSize(command_buffer_proxy_->route_id(), new_size));
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
    base::Bind(&CefMediaGpuProxy::OnFlushedTask, weak_ptr_));
}

void CefMediaGpuProxy::OnEndOfStream() {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnEndOfStreamTask, weak_ptr_));
}

void CefMediaGpuProxy::OnResolutionChanged(int width, int height) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnResolutionChangedTask, weak_ptr_, width, height));
}

void CefMediaGpuProxy::OnHaveEnough() {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnHaveEnoughTask, weak_ptr_));
}

void CefMediaGpuProxy::OnAudioTimeUpdate(int64_t pts) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::UpdateAudioTimeTask, weak_ptr_, pts));
}

void CefMediaGpuProxy::OnVideoTimeUpdate(int64_t pts) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::UpdateVideoTimeTask, weak_ptr_, pts));
}

void CefMediaGpuProxy::OnFrameCaptured(CefGpuMediaMsg_Frame_Params params) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::OnFrameCapturedTask, weak_ptr_, params));
}

void CefMediaGpuProxy::OnReleaseVideoBuffer(int32_t id) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::ReleaseVideoBufferTask, weak_ptr_, id));
}

void CefMediaGpuProxy::OnReleaseAudioBuffer(int32_t id) {
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaGpuProxy::ReleaseAudioBufferTask, weak_ptr_, id));
}

// Video frame release callback
void CefMediaGpuProxy::ReleaseMailbox(const uint32_t frame_id, const gpu::SyncToken& token) {
  Send(new CefGpuMediaMsg_ReleaseFrame(command_buffer_proxy_->route_id(), frame_id));
}

// Media tasks
void CefMediaGpuProxy::UpdateAudioTimeTask(int64_t pts) {
  client_->OnPTSUpdate(pts);
  if (audio_stream_)
    audio_stream_->OnPTSUpdate(pts);
}

void CefMediaGpuProxy::UpdateVideoTimeTask(int64_t pts) {
  client_->OnPTSUpdate(pts);
  if (video_stream_)
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
  LOG(INFO) << "Last audio pts " << pts;
  client_->OnLastPTS(pts);
}

void CefMediaGpuProxy::OnLastVideoPTSTask(int64_t pts) {
  LOG(INFO) << "Last video pts " << pts;
  client_->OnLastPTS(pts);
}

void CefMediaGpuProxy::OnFlushedTask() {
  LOG(ERROR) << "Renderer flushed";
  client_->OnFlushed();
}

void CefMediaGpuProxy::OnEndOfStreamTask() {
  LOG(ERROR) << "Received end of stream";
  client_->OnEndOfStream();
}

void CefMediaGpuProxy::OnResolutionChangedTask(int width, int height) {
  LOG(ERROR) << "Resolution changed width=" << width << " height=" << height;
  client_->OnResolutionChanged(width, height);
}

void CefMediaGpuProxy::OnHaveEnoughTask() {
  LOG(ERROR) << "Received have enough";
  client_->OnHaveEnough();
}

void CefMediaGpuProxy::OnFrameCapturedTask(CefGpuMediaMsg_Frame_Params params) {
  if (params.mailbox.IsZero()) {
    client_->OnMediaGpuProxyError(CefMediaGpuProxy::Client::kRendererError);
    return;
  }

  gpu::MailboxHolder mailbox_holder[media::VideoFrame::kMaxPlanes] = {
    gpu::MailboxHolder(params.mailbox, gpu::SyncToken(), GL_TEXTURE_2D)
  };

  client_->OnFrameCaptured(
    media::VideoFrame::WrapNativeTextures(
      media::VideoPixelFormat::PIXEL_FORMAT_ARGB, mailbox_holder,
      base::Bind(&CefMediaGpuProxy::ReleaseMailbox, weak_ptr_, params.frame_id),
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

    LOG(INFO) << "VP9 delegate support: " << support;
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

    LOG(INFO) << "Opus delegate support:" << support;
    return support;
  }

  LOG(ERROR) << "No command_buffer_proxy";
  return false;
}
