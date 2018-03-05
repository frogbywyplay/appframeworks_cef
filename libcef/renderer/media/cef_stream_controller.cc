#include "base/memory/shared_memory.h"
#include "base/logging.h"

#include "content/child/child_thread_impl.h"
#include "content/child/thread_safe_sender.h"

#include "media/base/bind_to_current_loop.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/video_decoder_config.h"

#include "libcef/renderer/media/cef_stream_controller.h"

namespace {

  static const size_t kSharedMemorySegmentBytes = 100 << 10;

  static media::Decryptor::StreamType DemuxerTypeToDecryptorType(
    media::DemuxerStream::Type type)
  {
    if (type == media::DemuxerStream::AUDIO)
      return media::Decryptor::kAudio;
    else
      return media::Decryptor::kVideo;
  }

}

CefStreamController::SHMBuffer::SHMBuffer()
  : size_(0) {
  shm_.reset(NULL);
}

CefStreamController::SHMBuffer::~SHMBuffer() {
  shm_.reset(NULL);
}

bool CefStreamController::SHMBuffer::Allocate(
  size_t size, content::ThreadSafeSender* thread_safe_sender) {
  shm_ = content::ChildThreadImpl::AllocateSharedMemory(size, thread_safe_sender, NULL);
  size_ = size;

  return shm_.get() != NULL;
}

base::SharedMemory* CefStreamController::SHMBuffer::shm() {
  return shm_.get();
}

bool CefStreamController::SHMBuffer::Map() {
  return shm_->Map(size_);
}

void CefStreamController::SHMBuffer::Unmap() {
  shm_->Map(size_);
}

size_t CefStreamController::SHMBuffer::Size() {
  return size_;
}

CefStreamController::CefStreamController(media::DemuxerStream* stream,
					 size_t max_frame_count,
					 content::ThreadSafeSender* thread_safe_sender,
					 CefStreamController::Client* client)
  : state_(kReady),
    stream_(stream),
    decryptor_(NULL),
    is_encrypted_(false),
    max_frame_count_(max_frame_count),
    start_time_(base::TimeDelta::FromMilliseconds(0)),
    next_bitstream_buffer_id_(0),
    client_(client),
    thread_safe_sender_(thread_safe_sender) {

  read_cb_ = media::BindToCurrentLoop(
    base::Bind(&CefStreamController::OnRead, this));

  decrypt_cb_ = media::BindToCurrentLoop(
    base::Bind(&CefStreamController::OnDecrypt, this));

  stream_->EnableBitstreamConverter();

  switch (stream_->type()) {
    case media::DemuxerStream::AUDIO:
      is_encrypted_ = stream_->audio_decoder_config().is_encrypted();
      break;
    case media::DemuxerStream::VIDEO:
      is_encrypted_ = stream_->video_decoder_config().is_encrypted();
      break;
    default:
      break;
  }
}

CefStreamController::~CefStreamController() {
  Abort();
  read_cb_.Reset();
  decrypt_cb_.Reset();
}

void CefStreamController::SetDecryptor(media::Decryptor* decryptor) {
  decryptor_ = decryptor;

  decryptor_->RegisterNewKeyCB(
    DemuxerTypeToDecryptorType(stream_->type()),
    media::BindToCurrentLoop(base::Bind(&CefStreamController::OnKeyAdded, this))
    );
}

void CefStreamController::SetStartTime(base::TimeDelta start_time) {
  start_time_ = start_time;
}

void CefStreamController::Feed() {
  if (state_ == kStopped)
    state_ = kReady;

  Read();
}

void CefStreamController::Stop() {
  state_ = kStopped;

  while (pending_pts_.size() > 0)
    pending_pts_.pop();
}

void CefStreamController::Abort() {
  if (state_ == kDecrypting)
    decryptor_->CancelDecrypt(DemuxerTypeToDecryptorType(stream_->type()));

  state_ = kStopped;

  while (pending_pts_.size() > 0)
    pending_pts_.pop();
}

void CefStreamController::OnPTSUpdate(int64_t pts) {
  size_t count = 0;

  while (pending_pts_.size() > 0 && pending_pts_.front() <= pts) {
    pending_pts_.pop();
    count++;
  }

  if (pending_pts_.size() < max_frame_count_ && state_ == kBufferFull) {
    state_ = kReady;
    Read();
  }

  if (count > 0)
    client_->AddDecodedFrames(count);
}

void CefStreamController::ReleaseBuffer(uint32_t id) {
  std::map<int32_t, std::unique_ptr<SHMBuffer> >::iterator it =
    shared_buffers_.find(id);

  if (it != shared_buffers_.end()) {
    client_->AddDecodedBytes(it->second->Size());
    shared_buffers_.erase(it);
  }
}

void CefStreamController::Read() {
  if (state_ == kReady) {

    if (pending_pts_.size() >= max_frame_count_) {
      state_ = kBufferFull;
      return;
    }

    state_ = kReading;

    if (pending_buffer_.get()) {
      scoped_refptr<media::DecoderBuffer> buffer = pending_buffer_;
      pending_buffer_ = NULL;
      OnRead(media::DemuxerStream::kOk, buffer);
    } else {
      stream_->Read(read_cb_);
    }
  }
}

void CefStreamController::OnRead(media::DemuxerStream::Status status,
				 const scoped_refptr<media::DecoderBuffer>& buffer) {
  if (state_ != kReading)
    return;

  if (status == media::DemuxerStream::kOk) {
    if (buffer->end_of_stream() && pending_pts_.size() > 0)
      client_->OnLastPTS(pending_pts_.back());
    else if (is_encrypted_)
      Decrypt(buffer);
    else
      SendBuffer(buffer);
  } else if (status == media::DemuxerStream::kConfigChanged) {
    ConfigChanged();
  } else if (status != media::DemuxerStream::kAborted) {
    client_->OnError();
    Stop();
  }
}

void CefStreamController::Decrypt(const scoped_refptr<media::DecoderBuffer>& buffer) {
  if (decryptor_) {
    decryptor_->Decrypt(DemuxerTypeToDecryptorType(stream_->type()),
			buffer, decrypt_cb_);
    state_ = kDecrypting;
  } else {
    pending_buffer_ = buffer;
    state_ = kWaitingKey;
  }
}

void CefStreamController::OnDecrypt(media::Decryptor::Status status,
				    const scoped_refptr<media::DecoderBuffer>& buffer) {
  if (status == media::Decryptor::kSuccess && buffer.get() != NULL) {
    SendBuffer(buffer);
  } else if (status == media::Decryptor::kNoKey) {
    state_ = kWaitingKey;

    if (buffer.get() && buffer->data_size() > 0)
      pending_buffer_ = buffer;

    client_->OnNeedKey();
  } else {
    client_->OnError();
    Stop();
  }
}

void CefStreamController::SendBuffer(const scoped_refptr<media::DecoderBuffer>& buffer) {
  size_t size;
  SHMBuffer *shm_buffer = new SHMBuffer();
  int64_t pts = (buffer->timestamp() - start_time_).InMilliseconds();

  if (state_ != kReading && state_ != kDecrypting)
    return;

  size = buffer->data_size();

  if (!shm_buffer->Allocate(size, thread_safe_sender_) || !shm_buffer->Map()) {
    LOG(ERROR) << "Failed to allocate or map shared buffer";
    client_->OnError();
    Stop();
    return;
  }

  memcpy(shm_buffer->shm()->memory(), buffer->data(), size);

  media::BitstreamBuffer bitstream_buffer(
    next_bitstream_buffer_id_, shm_buffer->shm()->handle(), size, 0,
    base::TimeDelta::FromMilliseconds(pts));

  base::SharedMemoryHandle handle =
    client_->ShareBufferToGpuProcess(bitstream_buffer.handle());

  shm_buffer->Unmap();

  if (!base::SharedMemory::IsHandleValid(handle)) {
    // We did our best, keep going
    LOG(WARNING) << "Invalid shared buffer handle";
    delete shm_buffer;
    state_ = kReady;
    Read();
    return;
  }

  media::BitstreamBuffer buffer_to_send = bitstream_buffer;
  buffer_to_send.set_handle(handle);
  client_->SendBuffer(buffer_to_send);

  pending_pts_.push(pts);
  shared_buffers_[next_bitstream_buffer_id_].reset(shm_buffer);

  state_ = kReady;
  next_bitstream_buffer_id_ = (next_bitstream_buffer_id_ + 1) & 0x3FFFFFFF;

  Read();
}

void CefStreamController::ConfigChanged() {
  switch (stream_->type()) {
    case media::DemuxerStream::AUDIO:
      stream_->audio_decoder_config();
      break;
    case media::DemuxerStream::VIDEO:
      client_->VideoConfigurationChanged(
	stream_->video_decoder_config().coded_size());
      break;
    default:
      break;
  }

  state_ = kReady;

  Read();
}

void CefStreamController::OnKeyAdded() {
  if (state_ == kWaitingKey) {
    state_ = kReady;
    Read();
  }
}
