#ifndef CEF_LIBCEF_RENDERER_MEDIA_CEF_RENDERER_H_
# define CEF_LIBCEF_RENDERER_MEDIA_CEF_RENDERER_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"

#include "media/base/renderer.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_log.h"
#include "media/base/video_renderer_sink.h"
#include "media/base/demuxer_stream_provider.h"
#include "media/base/cdm_context.h"

#include "libcef/common/cef_display.h"

#include "include/cef_media_delegate.h"

class CefMediaDevice;

class CefMediaRenderer :
  public media::Renderer {
  public:

    typedef base::Callback<void(int)> StatisticsCB;
    typedef base::Callback<void(int64_t)> LastPTSCB;

    CefMediaRenderer(
      const scoped_refptr<media::MediaLog>& media_log,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const scoped_refptr<base::TaskRunner>& worker_task_runner,
      media::VideoRendererSink* video_renderer_sink,
      CefRefPtr<CefMediaDelegate>& delegate,
      const CefGetDisplayInfoCB& get_display_info_cb);

    virtual ~CefMediaRenderer();

    // Initializes the Renderer with |demuxer_stream_provider|, executing
    // |init_cb| upon completion.  If initialization fails, only |init_cb| (not
    // |error_cb|) should be called.  |demuxer_stream_provider| must be valid for
    // the lifetime of the Renderer object.  |init_cb| must only be run after this
    // method has returned.  Firing |init_cb| may result in the immediate
    // destruction of the caller, so it must be run only prior to returning.
    //
    // Permanent callbacks:
    // - |statistics_cb|: Executed periodically with rendering statistics.
    // - |buffering_state_cb|: Executed when buffering state is changed.
    // - |ended_cb|: Executed when rendering has reached the end of stream.
    // - |error_cb|: Executed if any error was encountered after initialization.
    // - |waiting_for_decryption_key_cb|: Executed whenever the key needed to
    //                                    decrypt the stream is not available.
    void Initialize(
      media::DemuxerStreamProvider* demuxer_stream_provider,
      const media::PipelineStatusCB& init_cb,
      const media::StatisticsCB& statistics_cb,
      const media::BufferingStateCB& buffering_state_cb,
      const base::Closure& ended_cb,
      const media::PipelineStatusCB& error_cb,
      const base::Closure& waiting_for_decryption_key_cb);

    // Associates the |cdm_context| with this Renderer for decryption (and
    // decoding) of media data, then fires |cdm_attached_cb| with the result.
    void SetCdm(media::CdmContext* cdm_context,
                const media::CdmAttachedCB& cdm_attached_cb);

    // The following functions must be called after Initialize().

    // Discards any buffered data, executing |flush_cb| when completed.
    void Flush(const base::Closure& flush_cb);

    // Starts rendering from |time|.
    void StartPlayingFrom(base::TimeDelta time);

    // Updates the current playback rate. The default playback rate should be 1.
    void SetPlaybackRate(double playback_rate);

    // Sets the output volume. The default volume should be 1.
    void SetVolume(float volume);

    // Returns the current media time.
    base::TimeDelta GetMediaTime();

    // Returns whether |this| renders audio.
    bool HasAudio();

    // Returns whether |this| renders video.
    bool HasVideo();

    // Signal the position and size of the video surface on screen
    void UpdateVideoSurface(int x, int y, int width, int height);

    void OnEndOfStream();
    void OnResolutionChanged(int width, int height);
    void OnVideoPTS(int64_t pts);
    void OnAudioPTS(int64_t pts);
    void OnHaveEnough();

  private:

    enum State {
      UNDEFINED,
      INITIALIZED,
      PLAYING,
      FLUSHING,
      STOPPING,
      STOPPED,
      ERROR,
    };

    bool CheckVideoConfiguration(media::DemuxerStream* stream);
    bool CheckAudioConfiguration(media::DemuxerStream* stream);

    void InitializeTask(media::DemuxerStream* video_stream,
                        media::DemuxerStream* audio_stream);
    void Cleanup();

    void OnStatistics(media::DemuxerStream::Type type, int size);
    void OnLastPTS(media::DemuxerStream::Type type, int64_t pts);
    void OnStop();

    const scoped_refptr<media::MediaLog>              media_log_;
    const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
    CefRefPtr<CefMediaDelegate>                       delegate_;
    media::VideoRendererSink*                         video_renderer_sink_;
    const CefGetDisplayInfoCB                         get_display_info_cb_;
    media::Decryptor*                                 decryptor_;

    media::PipelineStatusCB init_cb_;
    media::StatisticsCB     statistics_cb_;
    media::BufferingStateCB buffering_state_cb_;
    base::Closure           ended_cb_;
    media::PipelineStatusCB error_cb_;
    base::Closure           waiting_for_decryption_key_cb_;
    base::Closure           stop_cb_;
    media::CdmAttachedCB    cdm_attached_cb_;

    State   state_;
    bool    paused_;
    int64_t last_audio_pts_;
    int64_t last_video_pts_;
    int     x_;
    int     y_;
    int     width_;
    int     height_;
    float   volume_;
    float   rate_;
    bool    initial_video_hole_created_;

    base::TimeDelta           start_time_;
    base::TimeDelta           current_time_;
    media::PipelineStatistics stats_;

    CefMediaDevice *audio_renderer_;
    CefMediaDevice *video_renderer_;

    base::WeakPtr<CefMediaRenderer> weak_this_;
    base::WeakPtrFactory<CefMediaRenderer> weak_factory_;

    DISALLOW_COPY_AND_ASSIGN(CefMediaRenderer);
};

#endif /* !CEF_RENDERER.H */
