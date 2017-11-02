#include "media/base/demuxer_stream_provider.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decryptor.h"
#include "media/base/pipeline_status.h"
#include "media/base/timestamp_constants.h"
#include "media/base/video_decoder_config.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/buffering_state.h"
#include "media/base/renderer_client.h"
#include "base/bind.h"
#include "base/logging.h"
#include "content/child/child_thread_impl.h"

#include "libcef/renderer/media/cef_renderer.h"

CefMediaRenderer::CefMediaRenderer(
  const scoped_refptr<media::MediaLog>& media_log,
  const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
  const scoped_refptr<base::TaskRunner>& worker_task_runner,
  media::VideoRendererSink* video_renderer_sink,
  const CefGetDisplayInfoCB& get_display_info_cb)
  : media_log_(media_log),
    media_task_runner_(media_task_runner),
    video_renderer_sink_(video_renderer_sink),
    get_display_info_cb_(get_display_info_cb),
    decryptor_(NULL),
    client_(NULL),
    state_(UNDEFINED),
    paused_(false),
    last_pts_(-1),
    x_(0),
    y_(0),
    width_(0),
    height_(0),
    volume_(1),
    rate_(1),
    use_video_hole_(true),
    initial_video_hole_created_(false),
    start_time_(media::kNoTimestamp()),
    current_time_(media::kNoTimestamp()),
    media_gpu_proxy_(this, media_task_runner),
    thread_safe_sender_(content::ChildThreadImpl::current()->thread_safe_sender()),
    weak_factory_(this)
{
  weak_this_ = weak_factory_.GetWeakPtr();
}

CefMediaRenderer::~CefMediaRenderer()
{
  Cleanup();
}

void CefMediaRenderer::Initialize(
  media::DemuxerStreamProvider* demuxer_stream_provider,
  media::RendererClient* client,
  const media::PipelineStatusCB& init_cb)
{
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  media::DemuxerStream* video_stream = NULL;
  media::DemuxerStream* audio_stream = NULL;

  /* Save callbacks for future use */
  init_cb_ = media::BindToCurrentLoop(init_cb);
  client_ = client;
  stop_cb_ = media::BindToCurrentLoop(base::Bind(&CefMediaRenderer::OnStop, weak_this_));

  video_stream = demuxer_stream_provider->GetStream(media::DemuxerStream::VIDEO);
  audio_stream = demuxer_stream_provider->GetStream(media::DemuxerStream::AUDIO);

  media_task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaRenderer::InitializeTask, weak_this_, video_stream, audio_stream)
    );
}

// Associates the |cdm_context| with this Renderer for decryption (and
// decoding) of media data, then fires |cdm_attached_cb| with the result.
void CefMediaRenderer::SetCdm(media::CdmContext* cdm_context,
                              const media::CdmAttachedCB& cdm_attached_cb)
{
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (!cdm_context)
    goto err;

  decryptor_ = cdm_context->GetDecryptor();
  if (!decryptor_)
    goto err;

  media_gpu_proxy_.SetDecryptor(decryptor_);
  cdm_attached_cb.Run(true);
  return;

  err:
  cdm_attached_cb.Run(false);
}

// Discards any buffered data, executing |flush_cb| when completed.
void CefMediaRenderer::Flush(const base::Closure& flush_cb)
{
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  media_gpu_proxy_.Flush();
  state_ = FLUSHING;
}

// Starts rendering from |time|.
void CefMediaRenderer::StartPlayingFrom(base::TimeDelta time)
{
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (state_ == INITIALIZED || state_ == STOPPED)
    media_gpu_proxy_.Play();
  else if (state_ == PLAYING)
    media_gpu_proxy_.Reset();
  else if (state_ == FLUSHING)
    media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CefMediaRenderer::SendHaveEnough, weak_this_)
      );
  else
    return;

  state_ = PLAYING;
  start_time_ = time;
  current_time_ = time;
  last_pts_ = -1;
  if (!initial_video_hole_created_)
  {
    initial_video_hole_created_ = true;
    video_renderer_sink_->PaintSingleFrame(
      media::VideoFrame::CreateHoleFrame(gfx::Size()));
  }

  media_gpu_proxy_.Start(start_time_);
}

// Updates the current playback rate. The default playback rate should be 1.
void CefMediaRenderer::SetPlaybackRate(double playback_rate)
{
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (state_ != INITIALIZED && state_ != PLAYING)
    return;
  if (playback_rate == 0)
  {
    if (!paused_)
    {
      paused_ = true;
      media_gpu_proxy_.Pause();
    }
    return;
  }
  if (paused_)
  {
    paused_ = false;
    media_gpu_proxy_.Resume();
  }
  if (rate_ != playback_rate)
    media_gpu_proxy_.SetPlaybackRate(playback_rate);
  rate_ = playback_rate;
}

// Sets the output volume. The default volume should be 1.
void CefMediaRenderer::SetVolume(float volume)
{
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (state_ != INITIALIZED && state_ != STOPPED && state_ != PLAYING)
    return;
  if (volume != volume_)
  {
    volume_ = volume;
    media_gpu_proxy_.SetVolume(volume * 100);
  }
}

// Returns the current media time.
base::TimeDelta CefMediaRenderer::GetMediaTime()
{
  return current_time_;
}

// Returns whether |this| renders audio.
bool CefMediaRenderer::HasAudio()
{
  return media_gpu_proxy_.HasAudio();
}

// Returns whether |this| renders video.
bool CefMediaRenderer::HasVideo()
{
  return media_gpu_proxy_.HasVideo();
}

// Signal the position and size of the video surface on screen
void CefMediaRenderer::UpdateVideoSurface(int x, int y, int width, int height)
{
  if (state_ == UNDEFINED)
    return;

  if (x != x_ || y != y_ || width != width_ || height != height_)
  {
    CefDisplayInfo display_info = get_display_info_cb_.Run();

    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
    media_gpu_proxy_.SetVideoPlan(x, y, width, height, display_info.width,
				  display_info.height);
  }
}

// Media GPU proxy client impl
void CefMediaRenderer::OnFlushed()
{
  if (!flush_cb_.is_null()) {
    flush_cb_.Run();
    flush_cb_.Reset();
  }
}

void CefMediaRenderer::OnStatistics(media::DemuxerStream::Type type, int size)
{
  if (type == media::DemuxerStream::VIDEO)
  {
    stats_.video_bytes_decoded += size;
    stats_.video_frames_decoded++;
  }
  else if (type == media::DemuxerStream::AUDIO)
  {
    stats_.audio_bytes_decoded += size;
  }

  if (client_)
    client_->OnStatisticsUpdate(stats_);
}

void CefMediaRenderer::OnEndOfStream()
{
  if (state_ != FLUSHING && state_ != STOPPED)
  {
    if (client_)
      client_->OnEnded();
  }
}

void CefMediaRenderer::OnResolutionChanged(int width, int height)
{
  gfx::Size new_size(width, height);

  if (video_renderer_sink_ && use_video_hole_) {
    LOG(INFO) << "Drawing video hole";
    video_renderer_sink_->PaintSingleFrame(
      media::VideoFrame::CreateHoleFrame(new_size));
  }

  if (client_) {
    client_->OnVideoNaturalSizeChange(new_size);
    client_->OnVideoOpacityChange(true);
  }
}

void CefMediaRenderer::OnPTSUpdate(int64_t pts)
{
  current_time_ = start_time_ + base::TimeDelta::FromMilliseconds(pts);

  if (last_pts_ > 0 && last_pts_ <= pts)
    OnEndOfStream();
}

void CefMediaRenderer::OnHaveEnough()
{
  media_task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&CefMediaRenderer::SendHaveEnough, weak_this_)
    );
}

void CefMediaRenderer::OnLastPTS(int64_t pts)
{
  if (pts > last_pts_)
    last_pts_ = pts;
}

void CefMediaRenderer::OnFrameCaptured(const scoped_refptr<media::VideoFrame>& frame)
{
  if (video_renderer_sink_)
    video_renderer_sink_->PaintSingleFrame(frame);
}

void CefMediaRenderer::OnMediaGpuProxyError(CefMediaGpuProxy::Client::Error error)
{
  LOG(ERROR) << "Media GPU proxy error : " << error;

  if (client_)
    client_->OnError(media::PipelineStatus::PIPELINE_ERROR_DECODE);
}

void CefMediaRenderer::OnNeedKey() {
  client_->OnWaitingForDecryptionKey();
}

// Private methods
void CefMediaRenderer::InitializeTask(media::DemuxerStream* video_stream,
                                      media::DemuxerStream* audio_stream)
{
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (!media_gpu_proxy_.InitializeRenderer(video_stream, audio_stream,
					   thread_safe_sender_, use_video_hole_)) {
    init_cb_.Run(media::PipelineStatus::PIPELINE_ERROR_INITIALIZATION_FAILED);
    return;
  }

  if (decryptor_)
    media_gpu_proxy_.SetDecryptor(decryptor_);

  state_ = INITIALIZED;
  init_cb_.Run(media::PipelineStatus::PIPELINE_OK);
}

void CefMediaRenderer::Cleanup()
{
  if (state_ != STOPPED)
    media_gpu_proxy_.Stop();
  media_gpu_proxy_.CleanupRenderer();
  state_ = UNDEFINED;
  decryptor_ = NULL;
}

void CefMediaRenderer::OnStop()
{
  media_gpu_proxy_.Stop();
}

void CefMediaRenderer::SendHaveEnough()
{
  if (client_) {
    client_->OnBufferingStateChange(media::BUFFERING_HAVE_ENOUGH);
  }
}
