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

#include "libcef_dll/cpptoc/media_event_callback_cpptoc.h"


namespace {

// MEMBER FUNCTIONS - Body may be edited by hand.

void CEF_CALLBACK media_event_callback_end_of_stream(
    struct _cef_media_event_callback_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaEventCallbackCppToC::Get(self)->EndOfStream();
}

void CEF_CALLBACK media_event_callback_resolution_changed(
    struct _cef_media_event_callback_t* self, int width, int height) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaEventCallbackCppToC::Get(self)->ResolutionChanged(
      width,
      height);
}

void CEF_CALLBACK media_event_callback_video_pts(
    struct _cef_media_event_callback_t* self, int64 pts) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaEventCallbackCppToC::Get(self)->VideoPTS(
      pts);
}

void CEF_CALLBACK media_event_callback_audio_pts(
    struct _cef_media_event_callback_t* self, int64 pts) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaEventCallbackCppToC::Get(self)->AudioPTS(
      pts);
}

void CEF_CALLBACK media_event_callback_have_enough(
    struct _cef_media_event_callback_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;

  // Execute
  CefMediaEventCallbackCppToC::Get(self)->HaveEnough();
}

}  // namespace


// CONSTRUCTOR - Do not edit by hand.

CefMediaEventCallbackCppToC::CefMediaEventCallbackCppToC() {
  GetStruct()->end_of_stream = media_event_callback_end_of_stream;
  GetStruct()->resolution_changed = media_event_callback_resolution_changed;
  GetStruct()->video_pts = media_event_callback_video_pts;
  GetStruct()->audio_pts = media_event_callback_audio_pts;
  GetStruct()->have_enough = media_event_callback_have_enough;
}

template<> CefRefPtr<CefMediaEventCallback> CefCppToC<CefMediaEventCallbackCppToC,
    CefMediaEventCallback, cef_media_event_callback_t>::UnwrapDerived(
    CefWrapperType type, cef_media_event_callback_t* s) {
  NOTREACHED() << "Unexpected class type: " << type;
  return NULL;
}

#ifndef NDEBUG
template<> base::AtomicRefCount CefCppToC<CefMediaEventCallbackCppToC,
    CefMediaEventCallback, cef_media_event_callback_t>::DebugObjCt = 0;
#endif

template<> CefWrapperType CefCppToC<CefMediaEventCallbackCppToC,
    CefMediaEventCallback, cef_media_event_callback_t>::kWrapperType =
    WT_MEDIA_EVENT_CALLBACK;
