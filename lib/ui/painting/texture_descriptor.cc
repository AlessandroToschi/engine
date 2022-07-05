#include "flutter/lib/ui/painting/texture_descriptor.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"

#define TEXTURE_MODE_ID 0
#define TEXTURE_MODE_POINTER 1

namespace flutter {
std::shared_ptr<TextureDescriptor> TextureDescriptor::Init(
    const tonic::Int64List& raw_values) {
  const auto texture_mode = raw_values[0];
  switch (texture_mode) {
    case TEXTURE_MODE_ID:
      return std::shared_ptr<TextureDescriptor>(
          new AndroidTextureDescriptor(raw_values));
      break;

    case TEXTURE_MODE_POINTER:
      return std::shared_ptr<TextureDescriptor>(
          new IOSTextureDescriptor(raw_values));
      break;

    default:
      return std::shared_ptr<TextureDescriptor>(nullptr);
  }
}

TextureDescriptor::TextureDescriptor(const tonic::Int64List& raw_values) {
  raw_texture_ = raw_values[1];
  width_ = static_cast<int32_t>(raw_values[2]);
  height_ = static_cast<int32_t>(raw_values[3]);
  switch (raw_values[4]) {
    case ImageDescriptor::PixelFormat::kRGBA8888:
      color_type_ = SkColorType::kRGBA_8888_SkColorType;
      break;
    case ImageDescriptor::PixelFormat::kBGRA8888:
      color_type_ = SkColorType::kBGRA_8888_SkColorType;
      break;
    default:
      color_type_ = SkColorType::kUnknown_SkColorType;
      break;
  }
}

AndroidTextureDescriptor::AndroidTextureDescriptor(
    const tonic::Int64List& raw_values)
    : TextureDescriptor(raw_values) {}

GrBackendTexture AndroidTextureDescriptor::backendTexure() const {
#ifdef SK_GL
  // GL_TEXTURE_EXTERNAL_OES 0x8D65
  // GL_TEXTURE_2D 0x0DE1
  // GL_RGBA8_OES 0x8058
  // GL_RGBA8 0x8058
  const GrGLTextureInfo texture_info{
      0x8D65, static_cast<GrGLuint>(raw_texture_), 0x8058};
#else
  const GrMockTextureInfo texture_info;
#endif
  return GrBackendTexture{static_cast<int>(width_), static_cast<int>(height_),
                          GrMipMapped::kNo, texture_info};
}

IOSTextureDescriptor::IOSTextureDescriptor(const tonic::Int64List& raw_values)
    : TextureDescriptor(raw_values) {}

GrBackendTexture IOSTextureDescriptor::backendTexure() const {
#ifdef SK_METAL
  GrMtlTextureInfo texture_info;
  texture_info.fTexture =
      sk_cfp<const void*>(reinterpret_cast<const void*>(raw_texture_));
#else
  const GrMockTextureInfo texture_info;
#endif
  return GrBackendTexture{static_cast<int>(width_), static_cast<int>(height_),
                          GrMipMapped::kNo, texture_info};
}

}  // namespace flutter
