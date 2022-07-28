// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/compositing/scene.h"

#include "flutter/fml/make_copyable.h"
#include "flutter/fml/trace_event.h"
#include "flutter/lib/ui/painting/image.h"
#include "flutter/lib/ui/painting/picture.h"
#include "flutter/lib/ui/painting/render_surface.h"
#include "flutter/lib/ui/ui_dart_state.h"
#include "flutter/lib/ui/window/platform_configuration.h"
#include "flutter/lib/ui/window/window.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"

namespace flutter {

IMPLEMENT_WRAPPERTYPEINFO(ui, Scene);

#define FOR_EACH_BINDING(V) \
  V(Scene, toImage)         \
  V(Scene, renderToSurface) \
  V(Scene, dispose)

DART_BIND_ALL(Scene, FOR_EACH_BINDING)

void Scene::create(Dart_Handle scene_handle,
                   std::shared_ptr<flutter::Layer> rootLayer,
                   uint32_t rasterizerTracingThreshold,
                   bool checkerboardRasterCacheImages,
                   bool checkerboardOffscreenLayers) {
  auto scene = fml::MakeRefCounted<Scene>(
      std::move(rootLayer), rasterizerTracingThreshold,
      checkerboardRasterCacheImages, checkerboardOffscreenLayers);
  scene->AssociateWithDartWrapper(scene_handle);
}

Scene::Scene(std::shared_ptr<flutter::Layer> rootLayer,
             uint32_t rasterizerTracingThreshold,
             bool checkerboardRasterCacheImages,
             bool checkerboardOffscreenLayers) {
  // Currently only supports a single window.
  auto viewport_metrics = UIDartState::Current()
                              ->platform_configuration()
                              ->get_window(0)
                              ->viewport_metrics();

  layer_tree_ = std::make_unique<LayerTree>(
      SkISize::Make(viewport_metrics.physical_width,
                    viewport_metrics.physical_height),
      static_cast<float>(viewport_metrics.device_pixel_ratio));
  layer_tree_->set_root_layer(std::move(rootLayer));
  layer_tree_->set_rasterizer_tracing_threshold(rasterizerTracingThreshold);
  layer_tree_->set_checkerboard_raster_cache_images(
      checkerboardRasterCacheImages);
  layer_tree_->set_checkerboard_offscreen_layers(checkerboardOffscreenLayers);
}

Scene::~Scene() {}

void Scene::dispose() {
  layer_tree_.reset();
  ClearDartWrapper();
}

Dart_Handle Scene::toImage(uint32_t width,
                           uint32_t height,
                           Dart_Handle raw_image_callback) {
  TRACE_EVENT0("flutter", "Scene::toImage");

  if (!layer_tree_) {
    return tonic::ToDart("Scene did not contain a layer tree.");
  }

  auto picture = layer_tree_->Flatten(SkRect::MakeWH(width, height));
  if (!picture) {
    return tonic::ToDart("Could not flatten scene into a layer tree.");
  }

  return Picture::RasterizeToImage(picture, width, height, raw_image_callback);
}

void Scene::renderToSurface(int32_t width,
                            int32_t height,
                            fml::RefPtr<RenderSurface> render_surface,
                            Dart_Handle callback) {
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
       this, render_surface, snapshot_delegate]() {
        snapshot_delegate->DrawLayerToSurface(
            layer_tree_.get(), render_surface->get_offscreen_surface());
        fml::TaskRunner::RunNowOrPostTask(std::move(ui_task_runner),
                                          std::move(ui_task));
      });

  fml::TaskRunner::RunNowOrPostTask(std::move(raster_task_runner),
                                    std::move(raster_task));
}

std::unique_ptr<flutter::LayerTree> Scene::takeLayerTree() {
  return std::move(layer_tree_);
}

}  // namespace flutter
