#include "ipc/ipc_message_macros.h"
#include "ipc/param_traits_macros.h"

#include "media/gpu/ipc/common/media_param_traits.h"

#include "gpu/ipc/common/gpu_command_buffer_traits.h"

#include "media/base/audio_codecs.h"
#include "media/base/video_codecs.h"

#define IPC_MESSAGE_START MediaMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(media::VideoCodec,
			  media::VideoCodec::kVideoCodecMax)
IPC_ENUM_TRAITS_MAX_VALUE(media::AudioCodec,
			  media::AudioCodec::kAudioCodecMax)

IPC_STRUCT_BEGIN(CefGpuMediaMsg_Initialize_Params)
  IPC_STRUCT_MEMBER(int32_t, ipc_route_id)
  IPC_STRUCT_MEMBER(gfx::Size, size)
  IPC_STRUCT_MEMBER(media::VideoCodec, video_codec)
  IPC_STRUCT_MEMBER(media::AudioCodec, audio_codec)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(CefGpuMediaMsg_Initialize_Result)
  IPC_STRUCT_MEMBER(bool, success)
  IPC_STRUCT_MEMBER(bool, use_video_hole)
  IPC_STRUCT_MEMBER(size_t, max_video_frame_count)
  IPC_STRUCT_MEMBER(size_t, max_audio_frame_count)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(CefGpuMediaMsg_SetVideoPlan_Params)
  IPC_STRUCT_MEMBER(int, x)
  IPC_STRUCT_MEMBER(int, y)
  IPC_STRUCT_MEMBER(int, width)
  IPC_STRUCT_MEMBER(int, height)
  IPC_STRUCT_MEMBER(int, display_width)
  IPC_STRUCT_MEMBER(int, display_height)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(CefGpuMediaMsg_Frame_Params)
  IPC_STRUCT_MEMBER(gfx::Size, coded_size)
  IPC_STRUCT_MEMBER(gfx::Size, natural_size)
  IPC_STRUCT_MEMBER(gfx::Rect, visible_rect)
  IPC_STRUCT_MEMBER(int32_t, frame_id, 0) /* id of the frame on gpu side for release */
  IPC_STRUCT_MEMBER(gpu::Mailbox, mailbox)
  IPC_STRUCT_MEMBER(int64_t, pts)
IPC_STRUCT_END()

IPC_SYNC_MESSAGE_ROUTED0_1(CefGpuMediaMsg_HasVP9Support, bool)
IPC_SYNC_MESSAGE_ROUTED0_1(CefGpuMediaMsg_HasOpusSupport, bool)
IPC_SYNC_MESSAGE_ROUTED1_1(CefGpuMediaMsg_Initialize,
			   CefGpuMediaMsg_Initialize_Params,
			   CefGpuMediaMsg_Initialize_Result)
IPC_SYNC_MESSAGE_ROUTED0_0(CefGpuMediaMsg_Cleanup)

/* Renderer -> GPU */
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_Play)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_Pause)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_Resume)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_Stop)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_Flush)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_Reset)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_SetPlaybackRate, double /* rate */)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_SetVolume, float /* volume */)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_SetVideoPlan,
		    CefGpuMediaMsg_SetVideoPlan_Params)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_RenderVideo, media::BitstreamBuffer)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_RenderAudio, media::BitstreamBuffer)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_ReleaseFrame, int32_t /* frame_id */)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_UpdateSize, gfx::Size /* size */)

/* GPU -> Renderer */
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_ReleaseVideoBuffer, uint32_t /* id */)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_ReleaseAudioBuffer, uint32_t /* id */)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_Flushed)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_EndOfStream)
IPC_MESSAGE_ROUTED2(CefGpuMediaMsg_ResolutionChanged,
		    int /* width */, int /* height */)
IPC_MESSAGE_ROUTED0(CefGpuMediaMsg_HaveEnough)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_AudioTimeUpdate, int64_t /* pts */)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_VideoTimeUpdate, int64_t /* pts */)
IPC_MESSAGE_ROUTED1(CefGpuMediaMsg_FrameCaptured,
		    CefGpuMediaMsg_Frame_Params)
