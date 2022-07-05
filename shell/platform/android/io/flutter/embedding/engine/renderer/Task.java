package io.flutter.embedding.engine.renderer;

import androidx.annotation.Keep;

@Keep
public interface Task {
  @SuppressWarnings("unused")
  public void run();
}
