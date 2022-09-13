// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_SNAPSHOT_DELEGATE_H_
#define FLUTTER_LIB_UI_SNAPSHOT_DELEGATE_H_

#include <string>
#include "flutter/flow/layers/layer_tree.h"
#include "flutter/flow/layers/offscreen_surface.h"
#include "flutter/lib/ui/painting/texture_descriptor.h"

#include "flutter/display_list/display_list.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPromiseImageTexture.h"
#include "third_party/skia/include/gpu/GrContextThreadSafeProxy.h"
namespace flutter {

class SnapshotDelegate {
 public:
  virtual std::pair<sk_sp<SkImage>, std::string> MakeGpuImage(
      sk_sp<DisplayList> display_list,
      SkISize picture_size) = 0;

  virtual sk_sp<SkImage> MakeRasterSnapshot(
      std::function<void(SkCanvas*)> draw_callback,
      SkISize picture_size) = 0;

  virtual sk_sp<SkImage> MakeRasterSnapshot(sk_sp<SkPicture> picture,
                                            SkISize picture_size) = 0;

  virtual sk_sp<SkImage> ConvertToRasterImage(sk_sp<SkImage> image) = 0;

  virtual sk_sp<SkImage> UploadTexture(
      std::shared_ptr<TextureDescriptor>& descriptor) = 0;

  virtual sk_sp<SkSurface> MakeSurface(int32_t width,
                                       int32_t height,
                                       int64_t raw_texture) = 0;

  virtual bool DrawLayerToSurface(LayerTree* tree,
                                  OffscreenSurface* snapshot_surface) = 0;

  virtual RasterCache* GetRasterCache() = 0;

  virtual GrDirectContext* GetContext() = 0;
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_SNAPSHOT_DELEGATE_H_
