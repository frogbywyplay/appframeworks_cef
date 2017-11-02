#ifndef CEF_LIBCEF_RENDERER_MEDIA_CEF_STREAM_CONTROLLER_H_
# define CEF_LIBCEF_RENDERER_MEDIA_CEF_STREAM_CONTROLLER_H_

#include "base/callback.h"

#include "media/base/bitstream_buffer.h"
#include "media/base/demuxer_stream.h"
#include "media/base/decryptor.h"
#include "media/base/decoder_buffer.h"

#include "content/child/thread_safe_sender.h"

#include <queue>

class CefStreamController {
  public:

    class Client {
      public :

	virtual void OnLastPTS(int64_t pts) = 0;
	virtual void OnNeedKey() = 0;
	virtual void OnError() = 0;
	virtual base::SharedMemoryHandle ShareBufferToGpuProcess(
	  base::SharedMemoryHandle handle) = 0;
        virtual void SendBuffer(media::BitstreamBuffer& buffer) = 0;
	virtual void VideoConfigurationChanged(gfx::Size size) = 0;
    };

    CefStreamController(media::DemuxerStream* stream,
			size_t max_frame_count,
			content::ThreadSafeSender* thread_safe_sender,
		        Client* client);
    ~CefStreamController();

    void SetDecryptor(media::Decryptor* decryptor);
    void SetStartTime(base::TimeDelta start_time);
    void Feed();
    void Stop();
    void Abort();
    void OnPTSUpdate(int64_t pts);
    void ReleaseBuffer(uint32_t id);

  private:

    class SHMBuffer {
      public:
	SHMBuffer();
	~SHMBuffer();

	bool Allocate(size_t size, content::ThreadSafeSender* thread_safe_sender);
	base::SharedMemory* shm();
	bool Map();
	void Unmap();

      private:
	std::unique_ptr<base::SharedMemory> shm_;
	size_t size_;
    };

    void Read();
    void OnRead(media::DemuxerStream::Status status,
		const scoped_refptr<media::DecoderBuffer>& buffer);
    void Decrypt(const scoped_refptr<media::DecoderBuffer>& buffer);
    void OnDecrypt(media::Decryptor::Status status,
		   const scoped_refptr<media::DecoderBuffer>& buffer);
    void SendBuffer(const scoped_refptr<media::DecoderBuffer>& buffer);
    void ConfigChanged();
    void OnKeyAdded();

    enum State {
      kReady,
      kWaitingKey,
      kReading,
      kDecrypting,
      kBufferFull,
      kStopped
    } state_;

    media::DemuxerStream* stream_;
    media::Decryptor* decryptor_;

    media::DemuxerStream::ReadCB read_cb_;
    media::Decryptor::DecryptCB decrypt_cb_;

    bool is_encrypted_;
    size_t max_frame_count_;
    base::TimeDelta start_time_;

    scoped_refptr<media::DecoderBuffer> pending_buffer_;

    std::queue<int64_t> pending_pts_;
    std::map<int32_t, std::unique_ptr<SHMBuffer> > shared_buffers_;

    int32_t next_bitstream_buffer_id_;

    Client* client_;

    content::ThreadSafeSender* thread_safe_sender_;

    base::WeakPtrFactory<CefStreamController> weak_this_factory_;
};

#endif /* !CEF_LIBCEF_RENDERER_MEDIA_CEF_STREAM_CONTROLLER.H */
