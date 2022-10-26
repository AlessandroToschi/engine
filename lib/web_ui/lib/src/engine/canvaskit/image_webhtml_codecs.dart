// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library image_webhtml_codecs;

import 'dart:async';
import 'dart:typed_data';

import 'package:ui/ui.dart' as ui;
import '../dom.dart';

import 'canvaskit_api.dart';
import 'image.dart';

class CkHtmlImage implements ui.Codec {
  /// Decodes an image from a list of encoded bytes.
  CkHtmlImage.decodeFromBytes(this._bytes, this.src) {
    _imageElement = createDomHTMLImageElement();
    final DomBlob blob = createDomBlob([this._bytes]);
    final String url = domWindow.URL.createObjectURL(blob);
    _imageElement!.src = url;
  }

  CkHtmlImage.decodeFromUrl(this.src) {
    _imageElement = createDomHTMLImageElement();
    _imageElement!.src = src;
    _imageElement!.crossOrigin = 'anonymous';
  }

  DomHTMLImageElement? _imageElement;

  final String src;
  Uint8List? _bytes;

  bool _disposed = false;
  bool get debugDisposed => _disposed;

  bool _debugCheckIsNotDisposed() {
    assert(!_disposed, 'This image has been disposed.');
    return true;
  }

  @override
  void dispose() {
    assert(
      !_disposed,
      'Cannot dispose a codec that has already been disposed.',
    );
    _disposed = true;
  }

  @override
  int get frameCount {
    assert(_debugCheckIsNotDisposed());
    return 1;
  }

  @override
  int get repetitionCount {
    assert(_debugCheckIsNotDisposed());
    return 1;
  }

  @override
  Future<ui.FrameInfo> getNextFrame() async {
    assert(_debugCheckIsNotDisposed());

    final DomHTMLImageElement img = _imageElement!;
    
    await img.decode();

    final SkImage? skImage = canvasKit.MakeLazyImageFromTextureSource(
      _imageElement!,
      SkPartialImageInfo(
        alphaType: canvasKit.AlphaType.Premul,
        colorType: canvasKit.ColorType.RGBA_8888,
        colorSpace: SkColorSpaceSRGB,
        width: img.naturalWidth,
        height: img.naturalHeight,
      ),
    );

    final ui.FrameInfo currentFrame = AnimatedImageFrameInfo(
      const Duration(milliseconds: 10000000),
      CkImage(skImage!),
    );

    return Future<ui.FrameInfo>.value(currentFrame);
  }
}
