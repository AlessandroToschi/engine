// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_COMMON_SNAPSHOT_CONTROLLER_IMPELLER_H_
#define FLUTTER_SHELL_COMMON_SNAPSHOT_CONTROLLER_IMPELLER_H_

#include "flutter/impeller/aiks/aiks_context.h"
#include "flutter/impeller/renderer/render_target.h"
#include "flutter/shell/common/snapshot_controller.h"

namespace flutter {

class SnapshotControllerImpeller : public SnapshotController {
 public:
  explicit SnapshotControllerImpeller(
      const SnapshotController::Delegate& delegate)
      : SnapshotController(delegate) {}

  sk_sp<DlImage> MakeRasterSnapshot(sk_sp<DisplayList> display_list,
                                    SkISize size) override;

  sk_sp<SkImage> ConvertToRasterImage(sk_sp<SkImage> image) override;

  sk_sp<DlImage> MakeFromTexture(int64_t raw_texture, SkISize size) override;

  std::unique_ptr<Surface> MakeOffscreenSurface(int64_t raw_texture,
                                                const SkISize& size) override;

 private:
  sk_sp<DlImage> DoMakeRasterSnapshot(const sk_sp<DisplayList>& display_list,
                                      SkISize size);

  FML_DISALLOW_COPY_AND_ASSIGN(SnapshotControllerImpeller);

  class OffscreenImpellerSurface : public Surface {
   public:
    OffscreenImpellerSurface(
        impeller::AiksContext* aiks_context,
        std::shared_ptr<impeller::RenderTarget> render_target);

    ~OffscreenImpellerSurface() override;

    bool IsValid() override;

    std::unique_ptr<SurfaceFrame> AcquireFrame(const SkISize& size) override;

    SkMatrix GetRootTransformation() const override;

    GrDirectContext* GetContext() override;

   private:
    impeller::AiksContext* _aiks_context;
    std::shared_ptr<impeller::RenderTarget> _render_target;
  };
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_COMMON_SNAPSHOT_CONTROLLER_IMPELLER_H_
