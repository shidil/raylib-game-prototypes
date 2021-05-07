#include <raylib.h>

namespace bomaqs {
Camera2D create2dCamera() {
  Camera2D camera = {0};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  return camera;
}
}  // namespace bomaqs
