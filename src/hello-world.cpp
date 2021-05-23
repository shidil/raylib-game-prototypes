#include <raylib.h>

#include <vector>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

using namespace std;

int main() {
  // Initialization
  //--------------------------------------------------------------------------------------

  Camera2D camera = {0};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello Raylib");
  SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

  // Main game loop
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    DrawFPS(10, 10);
    DrawText("Hello Raylib", 10, 200, 30, RED);

    EndMode2D();
    EndDrawing();
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow();  // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
