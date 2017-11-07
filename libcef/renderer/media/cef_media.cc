#include "media/base/media.h"

#include "include/cef_app.h"
#include "include/cef_media_delegate.h"

#include "libcef/common/content_client.h"
#include "libcef/renderer/media/cef_media_gpu_proxy.h"

bool media::PlatformHasOpusSupport() {
  return CefMediaGpuProxy::PlatformHasOpusSupport();
}

bool media::PlatformHasVP9Support() {
  return CefMediaGpuProxy::PlatformHasVP9Support();
}
