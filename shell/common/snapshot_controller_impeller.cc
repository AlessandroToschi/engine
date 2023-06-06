// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/common/snapshot_controller_impeller.h"

#include <algorithm>

#include "flutter/flow/surface.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/fml/trace_event.h"
#include "flutter/impeller/display_list/display_list_dispatcher.h"
#include "flutter/impeller/display_list/display_list_image_impeller.h"
#include "flutter/impeller/geometry/size.h"
#include "flutter/shell/common/snapshot_controller.h"

namespace flutter {

sk_sp<DlImage> SnapshotControllerImpeller::MakeFromTexture(int64_t raw_texture,
                                                           SkISize size) {
  const auto& delegate = GetDelegate();
  if (!delegate.GetSurface() || !delegate.GetAiksContext()) {
    return nullptr;
  }
  auto context = GetDelegate().GetSurface()->GetAiksContext();
  impeller::TextureDescriptor desc;
  desc.storage_mode = impeller::StorageMode::kHostVisible;
  desc.format = impeller::PixelFormat::kB8G8R8A8UNormInt;
  desc.size = {size.width(), size.height()};
  desc.mip_count = 1;
  auto texture = context->GetContext()->GetResourceAllocator()->WrapTexture(
      desc, raw_texture);
  return impeller::DlImageImpeller::Make(texture);
}

sk_sp<DlImage> SnapshotControllerImpeller::MakeRasterSnapshot(
    sk_sp<DisplayList> display_list,
    SkISize size) {
  sk_sp<DlImage> result;
  GetDelegate().GetIsGpuDisabledSyncSwitch()->Execute(
      fml::SyncSwitch::Handlers()
          .SetIfTrue([&] {
            // Do nothing.
          })
          .SetIfFalse(
              [&] { result = DoMakeRasterSnapshot(display_list, size); }));

  return result;
}

std::unique_ptr<Surface> SnapshotControllerImpeller::MakeOffscreenSurface(
    int64_t raw_texture,
    const SkISize& size) {
  const auto surface_size = impeller::ISize(size.width(), size.height());
  const auto aiks_context = GetDelegate().GetSurface()->GetAiksContext();
  auto context = aiks_context->GetContext();
  auto offscreen_render_target =
      impeller::RenderTarget::CreateOffscreenFromTexture(raw_texture, *context,
                                                         surface_size);
  return std::make_unique<OffscreenImpellerSurface>(aiks_context.get(),
                                                    offscreen_render_target);
}

sk_sp<DlImage> SnapshotControllerImpeller::DoMakeRasterSnapshot(
    const sk_sp<DisplayList>& display_list,
    SkISize size) {
  impeller::DisplayListDispatcher dispatcher;
  display_list->Dispatch(dispatcher);
  impeller::Picture picture = dispatcher.EndRecordingAsPicture();
  auto context = GetDelegate().GetAiksContext();
  if (context) {
    auto max_size = context->GetContext()
                        ->GetResourceAllocator()
                        ->GetMaxTextureSizeSupported();
    double scale_factor_x =
        static_cast<double>(max_size.width) / static_cast<double>(size.width());
    double scale_factor_y = static_cast<double>(max_size.height) /
                            static_cast<double>(size.height());
    double scale_factor =
        std::min(1.0, std::min(scale_factor_x, scale_factor_y));

    auto render_target_size = impeller::ISize(size.width(), size.height());

    // Scale down the render target size to the max supported by the
    // GPU if necessary. Exceeding the max would otherwise cause a
    // null result.
    if (scale_factor < 1.0) {
      render_target_size.width *= scale_factor;
      render_target_size.height *= scale_factor;
    }

    std::shared_ptr<impeller::Image> image =
        picture.ToImage(*context, render_target_size);
    if (image) {
      return impeller::DlImageImpeller::Make(image->GetTexture(),
                                             DlImage::OwningContext::kRaster);
    }
  }

  return nullptr;
}

sk_sp<SkImage> SnapshotControllerImpeller::ConvertToRasterImage(
    sk_sp<SkImage> image) {
  FML_UNREACHABLE();
}

SnapshotControllerImpeller::OffscreenImpellerSurface::OffscreenImpellerSurface(
    impeller::AiksContext* aiks_context,
    std::shared_ptr<impeller::RenderTarget> render_target)
    : _aiks_context(aiks_context), _render_target(render_target) {}

SnapshotControllerImpeller::OffscreenImpellerSurface::
    ~OffscreenImpellerSurface() = default;

bool SnapshotControllerImpeller::OffscreenImpellerSurface::IsValid() {
  return _aiks_context != nullptr;
}

std::unique_ptr<SurfaceFrame>
SnapshotControllerImpeller::OffscreenImpellerSurface::AcquireFrame(
    const SkISize& size) {
  const auto weak_render_target =
      std::weak_ptr<impeller::RenderTarget>(_render_target);
  const auto submit_callback = fml::MakeCopyable(
      [aiks_context = _aiks_context, weak_render_target = weak_render_target](
          SurfaceFrame& surface_frame, DlCanvas* canvas) mutable -> bool {
        if (!aiks_context) {
          return false;
        }

        const auto render_target = weak_render_target.lock();
        if (!render_target) {
          return false;
        }

        auto display_list = surface_frame.BuildDisplayList();
        if (!display_list) {
          FML_LOG(ERROR) << "Could not build display list for surface frame.";
          return false;
        }

        impeller::DisplayListDispatcher impeller_dispatcher;
        display_list->Dispatch(impeller_dispatcher);
        auto picture = impeller_dispatcher.EndRecordingAsPicture();

        aiks_context->Render(picture, *render_target);
        return true;
      });
  return std::make_unique<SurfaceFrame>(
      nullptr,                          // surface
      SurfaceFrame::FramebufferInfo{},  // framebuffer info
      submit_callback,                  // submit callback
      size,                             // frame size
      nullptr,                          // context result
      true);                            // display list fallback
}

SkMatrix
SnapshotControllerImpeller::OffscreenImpellerSurface::GetRootTransformation()
    const {
  return {};
}

GrDirectContext*
SnapshotControllerImpeller::OffscreenImpellerSurface::GetContext() {
  return nullptr;
}

}  // namespace flutter
