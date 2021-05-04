#include <raylib.h>

const int SCREEN_WIDTH = 360;
const int SCREEN_HEIGHT = 640;

int main() {
  // Initialization
  Camera2D camera = {0};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Word Game");
  SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

  // Main game loop
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);
    EndMode2D();
    EndDrawing();
  }

  // De-Initialization
  CloseWindow();  // Close window and OpenGL context

  return 0;
}
