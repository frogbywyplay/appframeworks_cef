#ifndef CEF_LIBCEF_RENDERER_MEDIA_GPU_CEF_GPU_VIDEO_CAPTURE_MANAGER_H_
# define CEF_LIBCEF_RENDERER_MEDIA_GPU_CEF_GPU_VIDEO_CAPTURE_MANAGER_H_

#include "base/callback.h"

#include "gpu/ipc/service/gpu_command_buffer_stub.h"
#include "gpu/command_buffer/common/mailbox.h"

#include "include/cef_media_delegate.h"

class CefGpuVideoCaptureManager {
  public:

    typedef base::Callback<void(
      int32_t frame_id, gpu::Mailbox mailbox, gfx::Size, gfx::Size, int64_t pts)> CaptureCb;

    CefGpuVideoCaptureManager(gfx::Size& size, gpu::GpuCommandBufferStub* stub,
			      const scoped_refptr<CefMediaDelegate>& delegate);
    ~CefGpuVideoCaptureManager();

    void CurrentPTS(int64_t pts);
    void CaptureFrame(CaptureCb on_capture_done);
    void ReleaseFrame(int32_t frame_id);
    void UpdateSize(gfx::Size& new_size);

  private:

    void CreateSurfaces();

    class Surface;

    std::queue<Surface*> available_surfaces_;
    std::map<uint32_t, Surface*> reserved_surfaces_;
    std::vector<std::unique_ptr<Surface> > surfaces_;

    gpu::GpuCommandBufferStub* stub_;
    scoped_refptr<CefMediaDelegate> delegate_;
    int64_t current_pts_;
    gfx::Size size_;
};

#endif /* !CEF_LIBCEF_RENDERER_MEDIA_GPU_CEF_GPU_VIDEO_CAPTURE_MANAGER.H */
