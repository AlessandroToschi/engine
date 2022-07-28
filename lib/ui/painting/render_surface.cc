// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/render_surface.h"
#include "flutter/flow/layers/layer_tree.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/lib/ui/ui_dart_state.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/dart_persistent_value.h"

namespace flutter {

static void RenderSurface_constructor(Dart_NativeArguments args) {
  UIDartState::ThrowIfUIOperationsProhibited();
  DartCallConstructor(&RenderSurface::Create, args);
}

IMPLEMENT_WRAPPERTYPEINFO(ui, RenderSurface);

#define FOR_EACH_BINDING(V) \
  V(RenderSurface, setup)   \
  V(RenderSurface, raw_texture)

FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

void RenderSurface::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register(
      {{"RenderSurface_constructor", RenderSurface_constructor, 2, true},
       FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
}

fml::RefPtr<RenderSurface> RenderSurface::Create(int64_t raw_texture) {
  return fml::MakeRefCounted<RenderSurface>(raw_texture);
}

void RenderSurface::setup(int32_t width, int32_t height, Dart_Handle callback) {
  auto* dart_state = UIDartState::Current();
  const auto ui_task_runner = dart_state->GetTaskRunners().GetUITaskRunner();
  const auto raster_task_runner =
      dart_state->GetTaskRunners().GetRasterTaskRunner();
  auto persistent_callback =
      std::make_unique<tonic::DartPersistentValue>(dart_state, callback);
  const auto snapshot_delegate = dart_state->GetSnapshotDelegate();

  const auto ui_task = fml::MakeCopyable(
      [persistent_callback = std::move(persistent_callback)]() mutable {
        auto dart_state = persistent_callback->dart_state().lock();
        if (!dart_state) {
          return;
        }
        tonic::DartState::Scope scope(dart_state);
        tonic::DartInvoke(persistent_callback->Get(), {});
        persistent_callback.reset();
      });

  const auto raster_task = fml::MakeCopyable(
      [ui_task = std::move(ui_task), ui_task_runner = std::move(ui_task_runner),
       snapshot_delegate = std::move(snapshot_delegate), width, height,
       this]() {
        _surface = snapshot_delegate->MakeSurface(width, height, _raw_texture);
        _raster_cache = snapshot_delegate->GetRasterCache();
        _context = snapshot_delegate->GetContext();
        _color_space = SkColorSpace::MakeSRGB();
#if SK_GL
        const auto render_target = _surface->getBackendRenderTarget(
            SkSurface::kDiscardWrite_BackendHandleAccess);
        if (render_target.isValid()) {
          GrGLFramebufferInfo info;
          render_target.getGLFramebufferInfo(&info);
          _raw_texture = info.fFBOID;
        }
#endif
        fml::TaskRunner::RunNowOrPostTask(std::move(ui_task_runner),
                                          std::move(ui_task));
      });

  fml::TaskRunner::RunNowOrPostTask(std::move(raster_task_runner),
                                    std::move(raster_task));
}

int64_t RenderSurface::raw_texture() {
  return _raw_texture;
}

SkCanvas* RenderSurface::get_canvas() {
  return _surface->getCanvas();
}

RasterCache* RenderSurface::get_raster_cache() {
  return _raster_cache;
}

GrDirectContext* RenderSurface::get_context() {
  return _context;
}

SkColorSpace* RenderSurface::get_color_space() {
  return _color_space.get();
}

RenderSurface::RenderSurface(int64_t raw_texture)
    : _surface{nullptr},
      _raw_texture{raw_texture},
      _raster_cache{nullptr},
      _context{nullptr},
      _color_space{nullptr} {}

RenderSurface::~RenderSurface() {}

}  // namespace flutter
