#ifndef FLUTTER_LIB_UI_PAINTING_SURFACE_H_
#define FLUTTER_LIB_UI_PAINTING_SURFACE_H_

#include "flutter/flow/layers/offscreen_surface.h"
#include "flutter/lib/ui/dart_wrapper.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace tonic {
class DartLibraryNatives;
}

namespace flutter {

class RenderSurface : public RefCountedDartWrappable<RenderSurface> {
  DEFINE_WRAPPERTYPEINFO();
  FML_FRIEND_MAKE_REF_COUNTED(RenderSurface);

 public:
  static fml::RefPtr<RenderSurface> Create(int64_t raw_texture);
  static void RegisterNatives(tonic::DartLibraryNatives* natives);

  ~RenderSurface() override;

  void setup(int32_t width, int32_t height, Dart_Handle callback);

  int64_t raw_texture();

  void dispose(Dart_Handle callback);

  OffscreenSurface* get_offscreen_surface();

 private:
  RenderSurface(int64_t raw_texture);

  std::unique_ptr<OffscreenSurface> _surface;
  int64_t _raw_texture;
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_PAINTING_SURFACE_H_
