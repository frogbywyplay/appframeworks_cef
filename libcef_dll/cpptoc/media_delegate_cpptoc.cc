// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//

#include "libcef_dll/cpptoc/media_delegate_cpptoc.h"
#include "libcef_dll/ctocpp/media_event_callback_ctocpp.h"


namespace {

// MEMBER FUNCTIONS - Body may be edited by hand.

void CEF_CALLBACK media_delegate_set_event_callback(
    struct _cef_media_delegate_t* self, cef_media_event_callback_t* event)
{
  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->SetEventCallback(CefMediaEventCallbackCToCpp::Wrap(event));
}
void CEF_CALLBACK media_delegate_stop(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->Stop();
}

void CEF_CALLBACK media_delegate_cleanup(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->Cleanup();
}

void CEF_CALLBACK media_delegate_pause(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->Pause();
}

void CEF_CALLBACK media_delegate_reset(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->Reset();
}

void CEF_CALLBACK media_delegate_play(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->Play();
}

void CEF_CALLBACK media_delegate_resume(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->Resume();
}

void CEF_CALLBACK media_delegate_set_speed(struct _cef_media_delegate_t* self,
    double rate) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->SetSpeed(
      rate);
}

void CEF_CALLBACK media_delegate_set_volume(struct _cef_media_delegate_t* self,
    float volume) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->SetVolume(
      volume);
}

void CEF_CALLBACK media_delegate_set_video_plan(
    struct _cef_media_delegate_t* self, int x, int y, int width, int height,
    unsigned int display_width, unsigned int display_height) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->SetVideoPlan(
      x,
      y,
      width,
      height,
      display_width,
      display_height);
}

int CEF_CALLBACK media_delegate_check_video_codec(
    struct _cef_media_delegate_t* self, cef_video_codec_t codec) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return 0;

  // Execute
  bool _retval = CefMediaDelegateCppToC::Get(self)->CheckVideoCodec(
      codec);

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK media_delegate_check_audio_codec(
    struct _cef_media_delegate_t* self, cef_audio_codec_t codec) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return 0;

  // Execute
  bool _retval = CefMediaDelegateCppToC::Get(self)->CheckAudioCodec(
      codec);

  // Return type: bool
  return _retval;
}

void CEF_CALLBACK media_delegate_flush(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->Flush();
}

int CEF_CALLBACK media_delegate_max_audio_sample_count(
    struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return 0;

  // Execute
  int _retval = CefMediaDelegateCppToC::Get(self)->MaxAudioSampleCount();

  // Return type: simple
  return _retval;
}

int CEF_CALLBACK media_delegate_max_video_sample_count(
    struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return 0;

  // Execute
  int _retval = CefMediaDelegateCppToC::Get(self)->MaxVideoSampleCount();

  // Return type: simple
  return _retval;
}

int CEF_CALLBACK media_delegate_open_audio(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return 0;

  // Execute
  bool _retval = CefMediaDelegateCppToC::Get(self)->OpenAudio();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK media_delegate_open_video(struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return 0;

  // Execute
  bool _retval = CefMediaDelegateCppToC::Get(self)->OpenVideo();

  // Return type: bool
  return _retval;
}

void CEF_CALLBACK media_delegate_close_audio(
    struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->CloseAudio();
}

void CEF_CALLBACK media_delegate_close_video(
    struct _cef_media_delegate_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaDelegateCppToC::Get(self)->CloseVideo();
}

int CEF_CALLBACK media_delegate_send_audio_buffer(
    struct _cef_media_delegate_t* self, char* buf, int size, int64 pts) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return -1;
  // Verify param: buf; type: simple_byaddr
  DCHECK(buf);
  if (!buf)
    return -1;

  // Execute
  int _retval = CefMediaDelegateCppToC::Get(self)->SendAudioBuffer(
      buf,
      size,
      pts);

  // Return type: simple
  return _retval;
}

int CEF_CALLBACK media_delegate_send_video_buffer(
    struct _cef_media_delegate_t* self, char* buf, int size, int64 pts) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return -1;
  // Verify param: buf; type: simple_byaddr
  DCHECK(buf);
  if (!buf)
    return -1;

  // Execute
  int _retval = CefMediaDelegateCppToC::Get(self)->SendVideoBuffer(
      buf,
      size,
      pts);

  // Return type: simple
  return _retval;
}

}  // namespace


// CONSTRUCTOR - Do not edit by hand.

CefMediaDelegateCppToC::CefMediaDelegateCppToC() {
  GetStruct()->set_event_callback = media_delegate_set_event_callback;
  GetStruct()->stop = media_delegate_stop;
  GetStruct()->cleanup = media_delegate_cleanup;
  GetStruct()->pause = media_delegate_pause;
  GetStruct()->reset = media_delegate_reset;
  GetStruct()->play = media_delegate_play;
  GetStruct()->resume = media_delegate_resume;
  GetStruct()->set_speed = media_delegate_set_speed;
  GetStruct()->set_volume = media_delegate_set_volume;
  GetStruct()->set_video_plan = media_delegate_set_video_plan;
  GetStruct()->check_video_codec = media_delegate_check_video_codec;
  GetStruct()->check_audio_codec = media_delegate_check_audio_codec;
  GetStruct()->flush = media_delegate_flush;
  GetStruct()->max_audio_sample_count = media_delegate_max_audio_sample_count;
  GetStruct()->max_video_sample_count = media_delegate_max_video_sample_count;
  GetStruct()->open_audio = media_delegate_open_audio;
  GetStruct()->open_video = media_delegate_open_video;
  GetStruct()->close_audio = media_delegate_close_audio;
  GetStruct()->close_video = media_delegate_close_video;
  GetStruct()->send_audio_buffer = media_delegate_send_audio_buffer;
  GetStruct()->send_video_buffer = media_delegate_send_video_buffer;
}

template<> CefRefPtr<CefMediaDelegate> CefCppToC<CefMediaDelegateCppToC,
    CefMediaDelegate, cef_media_delegate_t>::UnwrapDerived(CefWrapperType type,
    cef_media_delegate_t* s) {
  NOTREACHED() << "Unexpected class type: " << type;
  return NULL;
}

#ifndef NDEBUG
template<> base::AtomicRefCount CefCppToC<CefMediaDelegateCppToC,
    CefMediaDelegate, cef_media_delegate_t>::DebugObjCt = 0;
#endif

template<> CefWrapperType CefCppToC<CefMediaDelegateCppToC, CefMediaDelegate,
    cef_media_delegate_t>::kWrapperType = WT_MEDIA_DELEGATE;
