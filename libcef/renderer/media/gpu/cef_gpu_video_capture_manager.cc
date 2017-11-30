#include "base/logging.h"

#include "ui/gfx/buffer_types.h"
#include "ui/gfx/geometry/size.h"
#include "ui/ozone/public/native_pixmap.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/scoped_binders.h"
#include "ui/gl/gl_image_ozone_native_pixmap.h"

#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/command_buffer/service/mailbox_manager.h"

#include "libcef/renderer/media/gpu/cef_gpu_video_capture_manager.h"

namespace {

  gfx::Size ApplySampleRatio(cef_aspect_ratio_t *ratio, gfx::Size size)
  {
    return gfx::ScaleToCeiledSize(size, ratio->num, ratio->den);
  }

}

class CefGpuVideoCaptureManager::Surface {
  public:

    ~Surface() {
      mailbox_manager_->TextureDeleted(texture_ref_->texture());
    }

    void *GetNativeSurface() {
      return static_cast<void*>(pixmap_->GetNativeSurface());
    }

    uint32_t FrameId() {
      return texture_ref_->service_id();
    }

    gpu::Mailbox& Mailbox() {
      return mailbox_;
    }

    static Surface *Create(gpu::GpuCommandBufferStub* stub, gfx::Size& size) {
      scoped_refptr<gfx::GLImageOzoneNativePixmap> image(
	new gfx::GLImageOzoneNativePixmap(size, GL_RGBA));
      scoped_refptr<ui::NativePixmap> pixmap = ui::OzonePlatform::GetInstance()
	->GetSurfaceFactoryOzone()
	->CreateNativePixmap(gfx::kNullAcceleratedWidget,
			     size, gfx::BufferFormat::BGRA_8888,
			     gfx::BufferUsage::GPU_READ);
      gpu::gles2::ContextGroup* group = stub->decoder()->GetContextGroup();
      gpu::gles2::TextureManager* texture_manager = group->texture_manager();
      gpu::gles2::MailboxManager* mailbox_manager = group->mailbox_manager();
      GLuint texture_id;
      gpu::Mailbox mailbox;

      // Check that the allocation of the native surface succeeded
      if (!pixmap->GetNativeSurface()) {
	LOG(ERROR) << "Pixmap has no native surface";
	return NULL;
      }

      // Initialize egl image
      if (!image->Initialize(pixmap.get(), gfx::BufferFormat::BGRA_8888)) {
	LOG(ERROR) << "Failed to initialize egl image";
	return NULL;
      }

      // Create texture and bind it to the egl image
      glGenTextures(1, &texture_id);

      gfx::ScopedTextureBinder binder(GL_TEXTURE_2D, texture_id);
      scoped_refptr<gpu::gles2::TextureRef> texture_ref =
	gpu::gles2::TextureRef::Create(texture_manager, 0, texture_id);

      texture_manager->SetTarget(texture_ref.get(), GL_TEXTURE_2D);
      texture_manager->SetLevelInfo(texture_ref.get(),   // ref
				    GL_TEXTURE_2D,       // target
				    0,                   // level
				    GL_RGBA,             // internal_format
				    size.width(),        // width
				    size.height(),       // height
				    1,                   // depth
				    0,                   // border
				    GL_RGBA,             // format
				    GL_UNSIGNED_BYTE,    // type
				    gfx::Rect());        // cleared_rect
      image->BindTexImage(GL_TEXTURE_2D);
      texture_manager->SetParameteri(__func__, stub->decoder()->GetErrorState(),
				     texture_ref.get(), GL_TEXTURE_MAG_FILTER,
				     GL_LINEAR);
      texture_manager->SetParameteri(__func__, stub->decoder()->GetErrorState(),
				     texture_ref.get(), GL_TEXTURE_MIN_FILTER,
				     GL_LINEAR);

      stub->decoder()->RestoreActiveTextureUnitBinding(GL_TEXTURE_2D);

      // Create mailbox for the texture
      mailbox = gpu::Mailbox::Generate();
      mailbox_manager->ProduceTexture(mailbox, texture_ref->texture());

      return new Surface(mailbox_manager, pixmap, image, texture_ref, mailbox);
    }

  private:

    Surface(gpu::gles2::MailboxManager* mailbox_manager,
	    scoped_refptr<ui::NativePixmap>& pixmap,
	    scoped_refptr<gfx::GLImageOzoneNativePixmap>& image,
	    scoped_refptr<gpu::gles2::TextureRef>& texture_ref,
	    gpu::Mailbox& mailbox)
      : pixmap_(pixmap),
	image_(image),
	texture_ref_(texture_ref),
	mailbox_(mailbox),
	mailbox_manager_(mailbox_manager) {
    }

    // Native pixmap created by ozone platform
    scoped_refptr<ui::NativePixmap> pixmap_;
    // GL image interface for ozone platform
    scoped_refptr<gfx::GLImageOzoneNativePixmap> image_;
    // Texture corresponding to the egl image
    scoped_refptr<gpu::gles2::TextureRef> texture_ref_;
    // Unique name to identify the texture across processes
    gpu::Mailbox mailbox_;
    // Used to release the texture when the struct is destroyed
    gpu::gles2::MailboxManager* mailbox_manager_;
};

CefGpuVideoCaptureManager::CefGpuVideoCaptureManager(
  gfx::Size& size,
  gpu::GpuCommandBufferStub* stub,
  const scoped_refptr<CefMediaDelegate>& delegate)
  : stub_(stub),
    delegate_(delegate),
    current_pts_(0),
    size_(size) {
  CreateSurfaces();
}

CefGpuVideoCaptureManager::~CefGpuVideoCaptureManager() {
}

void CefGpuVideoCaptureManager::CurrentPTS(int64_t pts) {
  current_pts_ = pts;
}

void CefGpuVideoCaptureManager::CaptureFrame(CaptureCb on_capture_done) {
  if (available_surfaces_.size() > 0) {
    int width = 0, height = 0;
    cef_aspect_ratio_t aspect_ratio;
    Surface *surface = available_surfaces_.front();

    if (delegate_->CaptureFrame(
	  surface->GetNativeSurface(), &width, &height, &aspect_ratio)) {
      gfx::Size frame_size(width, height);
      gfx::Size natural_size = ApplySampleRatio(&aspect_ratio, frame_size);

      reserved_surfaces_[surface->FrameId()] = surface;
      available_surfaces_.pop();

      on_capture_done.Run(surface->FrameId(),
			  surface->Mailbox(),
			  frame_size,
			  natural_size,
			  current_pts_);
    }
  }
}

void CefGpuVideoCaptureManager::ReleaseFrame(int32_t frame_id) {
  std::map<uint32_t, Surface*>::iterator it =
    reserved_surfaces_.find(frame_id);

  if (it != reserved_surfaces_.end()) {
    available_surfaces_.push(it->second);
    reserved_surfaces_.erase(it);
  }
}

void CefGpuVideoCaptureManager::UpdateSize(gfx::Size& new_size) {
  size_ = new_size;

  /* TODO : Maybe not be so aggressive and handle displayed surfaces */

  while (available_surfaces_.size() > 0)
    available_surfaces_.pop();
  reserved_surfaces_.clear();
  surfaces_.clear();

  CreateSurfaces();
}

void CefGpuVideoCaptureManager::CreateSurfaces() {
  for (int i = 0; i < delegate_->MaxSurfaceCount(); i++) {
    std::unique_ptr<Surface> surface(Surface::Create(stub_, size_));

    if (!surface.get())
      break;

    available_surfaces_.push(surface.get());
    surfaces_.push_back(std::move(surface));
  }
}
