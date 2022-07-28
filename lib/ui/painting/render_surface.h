#ifndef FLUTTER_LIB_UI_PAINTING_SURFACE_H_
#define FLUTTER_LIB_UI_PAINTING_SURFACE_H_

#include "flutter/flow/layers/layer_tree.h"
#include "flutter/flow/raster_cache.h"
#include "flutter/lib/ui/dart_wrapper.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace tonic {
class DartLibraryNatives;
}

namespace flutter {

class RenderSurface : public RefCountedDartWrappable<RenderSurface>,
                      public RenderSurfaceProvider {
  DEFINE_WRAPPERTYPEINFO();
  FML_FRIEND_MAKE_REF_COUNTED(RenderSurface);

 public:
  static fml::RefPtr<RenderSurface> Create(int64_t raw_texture);
  static void RegisterNatives(tonic::DartLibraryNatives* natives);

  ~RenderSurface() override;

  void setup(int32_t width, int32_t height, Dart_Handle callback);

  int64_t raw_texture();

  SkCanvas* get_canvas() override;
  RasterCache* get_raster_cache() override;
  GrDirectContext* get_context() override;
  SkColorSpace* get_color_space() override;

 private:
  RenderSurface(int64_t raw_texture);

  sk_sp<SkSurface> _surface;
  int64_t _raw_texture;
  RasterCache* _raster_cache;
  GrDirectContext* _context;
  sk_sp<SkColorSpace> _color_space;
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_PAINTING_SURFACE_H_
