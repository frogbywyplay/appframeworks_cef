#include "media/base/media.h"

#include "include/cef_app.h"
#include "include/cef_media_delegate.h"

#include "libcef/common/content_client.h"

bool media::PlatformHasOpusSupport() {
  CefRefPtr<CefApp> application = CefContentClient::Get()->application();

  if (!application.get())
    return false;

  CefRefPtr<CefMediaDelegate> delegate = application->GetMediaDelegate();

  if (!delegate.get())
    return false;

  return delegate->HasOpusSupport();
}

bool media::PlatformHasVP9Support() {
  CefRefPtr<CefApp> application = CefContentClient::Get()->application();

  if (!application.get())
    return false;

  CefRefPtr<CefMediaDelegate> delegate = application->GetMediaDelegate();

  if (!delegate.get())
    return false;

  return delegate->HasVP9Support();
}
