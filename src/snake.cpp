#include <raylib.h>

#include <vector>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450
#define SNAKE_SCALE 5
using namespace std;

void drawSnake(vector<Vector2>);
void moveSnake(vector<Vector2> &currentSnake, Vector2 direction);

int main() {
  // Initialization
  //--------------------------------------------------------------------------------------

  Camera2D camera = {0};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake Game");
  SetTargetFPS(10);  // Set our game to run at 144 frames-per-second
  //--------------------------------------------------------------------------------------

  vector<Vector2> positions = {
      (Vector2){2 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){3 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){4 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){5 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){6 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){7 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){8 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){9 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
  };

  Vector2 direction = {1, 0};

  // Main game loop
  while (!WindowShouldClose()) {
    if (IsKeyDown(KEY_RIGHT)) {
      direction.x = 1;
      direction.y = 0;
    } else if (IsKeyDown(KEY_LEFT)) {
      direction.x = -1;
      direction.y = 0;
    } else if (IsKeyDown(KEY_DOWN)) {
      direction.y = 1;
      direction.x = 0;
    } else if (IsKeyDown(KEY_UP)) {
      direction.y = -1;
      direction.x = 0;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);
    moveSnake(positions, direction);
    drawSnake(positions);
    EndMode2D();
    EndDrawing();
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow();  // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}

void moveSnake(vector<Vector2> &snake, Vector2 direction) {
  int head = 0;
  int tail = snake.size();
  Vector2 prev = {snake[0].x, snake[0].y};

  for (int i = head + 1; i < tail; i++) {
    Vector2 current = {snake[i].x, snake[i].y};
    snake[i].x = prev.x;
    snake[i].y = prev.y;
    prev = current;
  }
  snake[head].x += direction.x * SNAKE_SCALE;
  snake[head].y += direction.y * SNAKE_SCALE;
}

void drawSnake(vector<Vector2> positions) {
  for (int i = 0; i < positions.size(); i++) {
    DrawRectangle(positions[i].x, positions[i].y, SNAKE_SCALE, SNAKE_SCALE,
                  MAROON);
  }
}
