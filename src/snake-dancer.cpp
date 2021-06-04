#include <raylib.h>
#include <raymath.h>

#include <iostream>
#include <vector>

#include "utils/data-loader.hpp"

#define FRAME_RATE 60
#define CRAYOLA \
  CLITERAL(Color) { 185, 226, 140, 255 }
#define PURPLE_NAVY \
  CLITERAL(Color) { 91, 80, 122, 255 }
#define DARK_BULE_GRAY \
  CLITERAL(Color) { 91, 97, 138, 255 }
#define MAX_G_YELLOW \
  CLITERAL(Color) { 214, 216, 79, 255 }

using namespace std;

void draw_snake(vector<Vector2>);
void draw_beat_indicators(float progress);
void move_snake(vector<Vector2> &currentSnake, Vector2 direction);
void camera_follow_smooth(Camera2D *camera, Vector2 player, float delta, int width, int height);

const int SCREEN_WIDTH = 450;
const int SCREEN_HEIGHT = 800;
const int SNAKE_SCALE = 25;

int main() {
  // Initialization
  //--------------------------------------------------------------------------------------

  Camera2D camera = {0};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake Dancer");
  InitAudioDevice();

  // load resources
  Music bgm_music = bomaqs::load_music("Funky-Chiptune.mp3");
  bgm_music.looping = true;
  SetMusicVolume(bgm_music, 0.9f);
  PlayMusicStream(bgm_music);

  SetTargetFPS(FRAME_RATE);  // Set our game to run at 144 frames-per-second
  //--------------------------------------------------------------------------------------

  vector<Vector2> positions = {
      (Vector2){2 + SNAKE_SCALE, SCREEN_HEIGHT / 2}, (Vector2){3 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){4 + SNAKE_SCALE, SCREEN_HEIGHT / 2}, (Vector2){5 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){6 + SNAKE_SCALE, SCREEN_HEIGHT / 2}, (Vector2){7 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
      (Vector2){8 + SNAKE_SCALE, SCREEN_HEIGHT / 2}, (Vector2){9 + SNAKE_SCALE, SCREEN_HEIGHT / 2},
  };

  const float music_bpm = 129.0f;
  const float beat_duration = 60.0f / music_bpm;  // music_bpm;

  Vector2 prev_direction = {1, 0};
  float beat_timer = beat_duration;
  bool input_mode = false;
  Vector2 center_circle = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 75};

  // Main game loop
  while (!WindowShouldClose()) {
    UpdateMusicStream(bgm_music);

    Vector2 direction = {0, 0};
    float delta_time = GetFrameTime();

    beat_timer -= delta_time;

    auto beat_progress = 1 - (beat_timer / beat_duration);
    bool is_hit = CheckCollisionCircles((Vector2){center_circle.x * beat_progress, center_circle.y},
                                        20, center_circle, 25);
    auto key_pressed = GetKeyPressed();

    switch (key_pressed) {
      case KEY_UP: {
        direction = {0, -1};
        break;
      }
      case KEY_RIGHT: {
        direction = {1, 0};
        break;
      }
      case KEY_DOWN: {
        direction = {0, 1};
        break;
      }
      case KEY_LEFT: {
        direction = {-1, 0};
        break;
      }
      default:
        direction = prev_direction;
        break;
    }

    if (is_hit) {
      prev_direction = direction;
    } else {
      direction = prev_direction;
    }

    if (beat_timer <= 0) {
      beat_timer = beat_duration;
      move_snake(positions, direction);
    }

    // move_snake(positions, direction);
    // camera_follow_smooth(&camera, positions[0], delta_time, SCREEN_WIDTH, SCREEN_HEIGHT);

    BeginDrawing();
    ClearBackground(CRAYOLA);
    BeginMode2D(camera);

    draw_beat_indicators(beat_progress);
    draw_snake(positions);

    if (beat_timer == beat_duration) {
      DrawText("XX", 10, 10, 20, BLUE);
    }

    if (key_pressed) {
      if (is_hit) {
        DrawText("Hit", 100, 10, 20, DARKGREEN);
      } else {
        DrawText("Miss", 100, 10, 20, RED);
      }
    }

    DrawText(to_string(beat_timer).data(), 350, 10, 20, WHITE);

    EndMode2D();
    EndDrawing();
  }

  // De-Initialization
  UnloadMusicStream(bgm_music);
  //--------------------------------------------------------------------------------------
  CloseWindow();  // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}

void move_snake(vector<Vector2> &snake, Vector2 direction) {
  if (Vector2Length(direction) == 0) {
    return;
  }

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

void draw_snake(vector<Vector2> positions) {
  for (int i = 0; i < positions.size(); i++) {
    DrawRectangle(positions[i].x, positions[i].y, SNAKE_SCALE, SNAKE_SCALE, PURPLE_NAVY);
  }
}

void draw_beat_indicators(float progress) {
  DrawCircleLines(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 75, 25, GRAY);
  DrawCircle(SCREEN_WIDTH / 2 * progress, SCREEN_HEIGHT - 75, 20, DARK_BULE_GRAY);
  DrawCircle(SCREEN_WIDTH - (SCREEN_WIDTH / 2 * progress), SCREEN_HEIGHT - 75, 20, DARK_BULE_GRAY);
}

void camera_follow_smooth(Camera2D *camera, Vector2 player, float delta, int width, int height) {
  static float minSpeed = 30;
  static float minEffectLength = 10;
  static float fractionSpeed = 0.8f;

  camera->offset = (Vector2){width / 2.0f, height / 2.0f};
  Vector2 diff = Vector2Subtract(player, camera->target);
  float length = Vector2Length(diff);

  if (length > minEffectLength) {
    float speed = fmaxf(fractionSpeed * length, minSpeed);
    camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed * delta / length));
  }
}
