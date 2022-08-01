// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/image.h"

#include <algorithm>
#include <limits>

#include "flutter/lib/ui/painting/image_encoding.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"

namespace tonic {

inline Dart_Handle LookupNullableType(const std::string& library_name,
                                      const std::string& type_name) {
  auto library =
      Dart_LookupLibrary(DartConverter<std::string>::ToDart(library_name));
  if (LogIfError(library)) {
    return library;
  }
  auto type_string = DartConverter<std::string>::ToDart(type_name);
  if (LogIfError(type_string)) {
    return type_string;
  }
  auto type = Dart_GetNullableType(library, type_string, 0, nullptr);
  if (LogIfError(type)) {
    return type;
  }
  return type;
}

template <>
struct DartConverter<std::vector<fml::RefPtr<flutter::CanvasImage>>> {
  using ValueType = fml::RefPtr<flutter::CanvasImage>;
  static Dart_Handle ToDart(const std::vector<ValueType>& images) {
    Dart_Handle nullable_type = LookupNullableType("dart:ui", "_Image");
    if (Dart_IsError(nullable_type))
      return nullable_type;
    Dart_Handle list = Dart_NewListOfType(nullable_type, images.size());
    if (Dart_IsError(list))
      return list;
    for (size_t i = 0; i < images.size(); i++) {
      Dart_Handle dart_image = DartConverter<ValueType>::ToDart(images[i]);
      if (Dart_IsError(dart_image))
        return dart_image;
      Dart_Handle result = Dart_ListSetAt(list, i, dart_image);
      if (Dart_IsError(result))
        return result;
    }
    return list;
  }
};
}  // namespace tonic
namespace flutter {

typedef CanvasImage Image;

// Since _Image is a private class, we can't use IMPLEMENT_WRAPPERTYPEINFO
static const tonic::DartWrapperInfo kDartWrapperInfo_ui_Image = {
    "ui",
    "_Image",
    sizeof(Image),
};
const tonic::DartWrapperInfo& Image::dart_wrapper_info_ =
    kDartWrapperInfo_ui_Image;

#define FOR_EACH_BINDING(V) \
  V(Image, width)           \
  V(Image, height)          \
  V(Image, toByteData)      \
  V(Image, dispose)

FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

void CanvasImage::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({{"Image_FromTextures", Image::FromTextures, 1, true},
                     FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
}

CanvasImage::CanvasImage() = default;

CanvasImage::~CanvasImage() = default;

Dart_Handle CanvasImage::toByteData(int format, Dart_Handle callback) {
  return EncodeImage(this, format, callback);
}

void CanvasImage::dispose() {
  image_.reset();
  ClearDartWrapper();
}

size_t CanvasImage::GetAllocationSize() const {
  auto size = sizeof(this);
  if (image_) {
    size += image_->GetApproximateByteSize();
  }
  // The VM will assert if we set a value larger than or close to
  // std::numeric_limits<intptr_t>::max().
  // https://github.com/dart-lang/sdk/issues/49332
  return std::clamp(
      size, static_cast<size_t>(0),
      static_cast<size_t>(std::numeric_limits<intptr_t>::max() / 10));
}

void CanvasImage::FromTextures(Dart_NativeArguments args) {
  Dart_Handle texture_descriptor_list = Dart_GetNativeArgument(args, 0);

  auto* dart_state = UIDartState::Current();
  auto unref_queue = dart_state->GetSkiaUnrefQueue();
  auto snapshot_delegate = dart_state->GetSnapshotDelegate().unsafeGet();

  intptr_t count;
  Dart_ListLength(texture_descriptor_list, &count);

  std::vector<fml::RefPtr<CanvasImage>> images;

  for (intptr_t i = 0; i < count; i++) {
    auto dart_texture_descriptor = Dart_ListGetAt(texture_descriptor_list, i);
    auto raw_texture_descriptor = tonic::Int64List(dart_texture_descriptor);
    auto texture_descriptor = TextureDescriptor::Init(raw_texture_descriptor);
    auto _image = snapshot_delegate->UploadTexture(texture_descriptor);

    if (_image != nullptr) {
      auto dart_image = CanvasImage::Create();
      auto gpu_image = SkiaGPUObject{std::move(_image), unref_queue};
      dart_image->set_image(DlImageGPU::Make(std::move(gpu_image)));
      images.push_back(dart_image);
    } else {
      break;
    }
  }
  Dart_SetReturnValue(args, tonic::ToDart(images));
}
}  // namespace flutter
