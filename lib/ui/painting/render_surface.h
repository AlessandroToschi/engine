#ifndef FLUTTER_LIB_UI_PAINTING_SURFACE_H_
#define FLUTTER_LIB_UI_PAINTING_SURFACE_H_

#include <memory>

#include "flutter/flow/layers/offscreen_surface.h"
#include "flutter/lib/ui/dart_wrapper.h"

namespace flutter {

class RenderSurface : public RefCountedDartWrappable<RenderSurface> {
  DEFINE_WRAPPERTYPEINFO();
  FML_FRIEND_MAKE_REF_COUNTED(RenderSurface);

 public:
  static void Create(Dart_Handle dart_handle, int64_t raw_texture);

  void setup(int32_t width, int32_t height, Dart_Handle callback);
  void dispose(Dart_Handle callback);

  std::shared_ptr<OffscreenSurface> surface() { return _surface; }
  bool is_valid() { return _surface != nullptr; }

  ~RenderSurface() override;

 private:
  RenderSurface(int64_t raw_texture);

  std::shared_ptr<OffscreenSurface> _surface;
  int64_t _raw_texture;
};
}  // namespace flutter

#endif
