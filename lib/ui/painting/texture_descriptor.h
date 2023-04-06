#ifndef FLUTTER_LIB_UI_PAINTING_TEXTURE_DESCRIPTOR_H_
#define FLUTTER_LIB_UI_PAINTING_TEXTURE_DESCRIPTOR_H_

#include <memory>

#include "flutter/lib/ui/painting/image_descriptor.h"
#include "third_party/tonic/typed_data/typed_list.h"

namespace flutter {
class TextureDescriptor {
 public:
  virtual ~TextureDescriptor() = default;

  virtual GrBackendTexture backendTexure() const = 0;

  SkColorType colorType() const { return color_type_; }

  static std::shared_ptr<TextureDescriptor> Init(
      const tonic::Int64List& raw_values);

 protected:
  int64_t raw_texture_;
  int32_t width_;
  int32_t height_;
  SkColorType color_type_;

  TextureDescriptor(const tonic::Int64List& raw_values);

 private:
  TextureDescriptor() = delete;
};

class AndroidTextureDescriptor final : public TextureDescriptor {
 public:
  ~AndroidTextureDescriptor() override = default;

  GrBackendTexture backendTexure() const override;

  AndroidTextureDescriptor(const tonic::Int64List& raw_values);
};

class IOSTextureDescriptor final : public TextureDescriptor {
 public:
  ~IOSTextureDescriptor() override = default;

  GrBackendTexture backendTexure() const override;

  IOSTextureDescriptor(const tonic::Int64List& raw_values);
};
}  // namespace flutter

#endif
