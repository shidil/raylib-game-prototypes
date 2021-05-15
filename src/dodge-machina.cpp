#include <raylib.h>

#include <cmath>
#include <set>
#include <string>
#include <vector>

#include "utils/camera-2d.hpp"
#include "utils/data-loader.hpp"

#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 960
#define FRAME_RATE 60
#define PLAYER_RADIUS 20
#define BULLET_RADIUS 5
#define BULLET_FIRE_RATE_MIN 12
#define BULLET_FIRE_RATE_MAX 15
#define MAX_BULLETS 100
#define BULLET_VELOCITY 10
#define MAX_ENEMIES 4
#define DASHER_VELOCITY 8
#define HOMING_VELOCITY 4
#define DASHER_BOUNDS \
  CLITERAL(Rectangle) { 50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100 }
#define BULLET_BOUNDS \
  CLITERAL(Rectangle) { -50, -50, SCREEN_WIDTH + 100, SCREEN_HEIGHT + 100 }

// void draw(Vector2 player, Vector2 enemy);

enum EnemyType {
  SHOOTER,
  DASHER,
  HOMING,
};

enum ActorState {
  LIVE,
  DASHING,
  DEAD,
};

typedef struct {
  Vector2 position;
  Color color;
  Vector2 velocity;
} Bullet;

typedef struct {
  Vector2 position;
  Color color;
  Vector2 velocity;
  int fire_rate;
  EnemyType type;
  // int bullets_fired;
} Enemy;

typedef struct {
  Vector2 position;
  Color color;
  ActorState state;
} Player;

void draw_bullets(std::vector<Bullet> bullets);
void draw_enemies(std::vector<Enemy> enemies);
void update_bullets(std::vector<Bullet> &bullets);
Bullet create_bullet(Enemy enemy, Player player);
Enemy create_enemy(void);
Vector2 get_homing_velocity(Vector2 pos1, Vector2 pos2, int velocity);

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Dodge Machina");
  InitAudioDevice();

  // load resources
  Sound teleport_sfx = bomaqs::load_sound("teleport2.wav");
  Music bgm_music = bomaqs::load_music("n-Dimensions (Main Theme).mp3");
  bgm_music.looping = true;
  SetMusicVolume(bgm_music, 0.35f);

  PlayMusicStream(bgm_music);
  SetTargetFPS(60);

  unsigned long long int frames_count = 0;

  Player player = {.position = {.x = SCREEN_WIDTH / 2, .y = SCREEN_HEIGHT / 2},
                   .color = RED,
                   .state = LIVE};
  std::vector<Bullet> bullets;
  std::vector<Enemy> enemies;

  while (!WindowShouldClose()) {
    // Update
    UpdateMusicStream(bgm_music);

    if (player.state != DEAD) {
      frames_count += 1;
    }

    // Player collisions with bullets
    for (int i = 0; i < bullets.size(); i++) {
      if (CheckCollisionCircles(player.position, PLAYER_RADIUS,
                                bullets[i].position, BULLET_RADIUS)) {
        player.state = DEAD;
      }
    }

    // Player collisions with enemies
    for (int i = 0; i < enemies.size(); i++) {
      Rectangle enemy_rect = {
          .x = enemies[i].position.x,
          .y = enemies[i].position.y,
          .width = 20,
          .height = 20,
      };
      if (CheckCollisionCircleRec(player.position, PLAYER_RADIUS, enemy_rect)) {
        player.state = DEAD;
      }
    }

    // player input
    auto current_gesture = GetGestureDetected();
    if (player.state == LIVE && current_gesture == GESTURE_TAP) {
      auto touch_position = GetTouchPosition(0);

      PlaySoundMulti(teleport_sfx);
      player.position.x = touch_position.x;
      player.position.y = touch_position.y;
    }

    // Enemy collisions
    std::set<int> to_be_removed;
    for (int i = 0; i < enemies.size(); i++) {
      Rectangle enemy_rect_1 = {
          .x = enemies[i].position.x,
          .y = enemies[i].position.y,
          .width = 20,
          .height = 20,
      };
      for (int j = 1; j < enemies.size(); j++) {
        Rectangle enemy_rect_2 = {
            .x = enemies[j].position.x,
            .y = enemies[j].position.y,
            .width = 20,
            .height = 20,
        };
        if (CheckCollisionRecs(enemy_rect_1, enemy_rect_2)) {
          to_be_removed.insert(i);
          to_be_removed.insert(j);
        }
      }
    }
    for (auto idx : to_be_removed) {
      enemies.erase(enemies.begin() + idx);
    }

    if (player.state != DEAD) {
      // spawn enemy
      if (enemies.size() < MAX_ENEMIES &&
          frames_count % (FRAME_RATE * 4) == 0) {
        enemies.push_back(create_enemy());
      }

      // update enemy
      for (int i = 0; i < enemies.size(); i++) {
        Enemy *enemy = &enemies[i];

        switch (enemy->type) {
          case SHOOTER:
            if (frames_count == 0 || frames_count % enemy->fire_rate == 0) {
              if (bullets.size() < MAX_BULLETS) {
                bullets.push_back(create_bullet(*enemy, player));
              }
            }
            break;
          case DASHER: {
            // skip enemy that is already dashing
            if (enemy->velocity.x == 0 && enemy->velocity.y == 0) {
              auto vel = get_homing_velocity(player.position, enemy->position,
                                             DASHER_VELOCITY);
              enemy->velocity.x = vel.x;
              enemy->velocity.y = vel.y;
            }

            // check dasher bounds
            // TODO: tweak bound rect, may be check enemy rect center point
            // inside dasher bounds?
            Rectangle enemy_rect = {
                .x = enemy->position.x,
                .y = enemy->position.y,
                .width = 20,
                .height = 20,
            };

            enemy->position.x += enemy->velocity.x;
            enemy->position.y += enemy->velocity.y;

            if (!CheckCollisionRecs(DASHER_BOUNDS, enemy_rect)) {
              enemy->velocity.x = 0;
              enemy->velocity.y = 0;
            }

            break;
          }
          case HOMING: {
            auto vel = get_homing_velocity(player.position, enemy->position,
                                           HOMING_VELOCITY);
            enemy->velocity.x = vel.x;
            enemy->velocity.y = vel.y;

            enemy->position.x += enemy->velocity.x;
            enemy->position.y += enemy->velocity.y;
            break;
          }
          default:
            break;
        }
      }
    }

    // bullets
    update_bullets(bullets);

    //---- Draw
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw game world
    DrawCircle(player.position.x, player.position.y, PLAYER_RADIUS,
               player.color);
    draw_enemies(enemies);
    draw_bullets(bullets);
    // debug dasher bounds
    DrawRectangleLinesEx(DASHER_BOUNDS, 2, LIGHTGRAY);

    // game over
    if (player.state == DEAD) {
      DrawText("You Died!", (SCREEN_WIDTH / 2) - 100, (SCREEN_HEIGHT / 2) - 25,
               40, BLACK);
    }

    DrawFPS(10, 10);
    std::string score_text = "Score: ";
    score_text.append(std::to_string(frames_count / 5));
    DrawText(score_text.data(), SCREEN_WIDTH - 120, 10, 20, ORANGE);

    EndMode2D();
    EndDrawing();
    // draw(player, enemy);
  }

  //---- De-Init
  StopSoundMulti();
  UnloadMusicStream(bgm_music);
  UnloadSound(teleport_sfx);
  CloseAudioDevice();
  CloseWindow();
}

float coordinate_angle(Vector2 pos1, Vector2 pos2) {
  return atan2(pos1.y - pos2.y, pos1.x - pos2.x);
}

Vector2 get_homing_velocity(Vector2 pos1, Vector2 pos2, int velocity) {
  auto angle = coordinate_angle(pos1, pos2);
  return {(float)cos(angle) * velocity, (float)sin(angle) * velocity};
}

Bullet create_bullet(Enemy enemy, Player player) {
  Bullet bullet = {
      .position = enemy.position,
      .color = BLACK,
      .velocity =
          get_homing_velocity(player.position, enemy.position, BULLET_VELOCITY),
  };

  return bullet;
}

Color enemy_colors[3] = {DARKGREEN, BLUE, VIOLET};

Enemy create_enemy() {
  // Avoid overlapping enemy and player, as well as other enemies
  // Keep min x distance from other enemies and player
  // Some randonmess in fire rate and other timings
  // Enemy spawn probability
  float x = GetRandomValue(50, SCREEN_WIDTH - 50);
  float y = GetRandomValue(50, SCREEN_HEIGHT - 50);

  Enemy enemy = {
      .position = {x, y},
      .color = enemy_colors[GetRandomValue(0, 2)],
      .velocity = {0, 0},
      .fire_rate = GetRandomValue(BULLET_FIRE_RATE_MIN, BULLET_FIRE_RATE_MAX),
      .type = static_cast<EnemyType>(GetRandomValue(0, 0))};
  return enemy;
}

void update_bullets(std::vector<Bullet> &bullets) {
  // TODO: bullets that go out of screen bounds needs to removed
  for (int i = 0; i < bullets.size(); i++) {
    if (!CheckCollisionPointRec(bullets[i].position, BULLET_BOUNDS)) {
      bullets.erase(bullets.begin() + i);
    } else {
      bullets[i].position.x += bullets[i].velocity.x;
      bullets[i].position.y += bullets[i].velocity.y;
    }
  }
}

void draw_bullets(std::vector<Bullet> bullets) {
  for (int i = 0; i < bullets.size(); i++) {
    DrawCircle(bullets[i].position.x, bullets[i].position.y, BULLET_RADIUS,
               BLACK);
  }
}

void draw_enemies(std::vector<Enemy> enemies) {
  for (int i = 0; i < enemies.size(); i++) {
    DrawRectangle(enemies[i].position.x, enemies[i].position.y, 20, 20,
                  enemies[i].color);
  }
}

// void draw(Vector2 player, Vector2 enemy) {
//   BeginDrawing();
//   ClearBackground(RAYWHITE);

//   // Draw game world
//   DrawCircle(player.x, player.y, PLAYER_RADIUS, RED);
//   DrawRectangle(enemy.x, enemy.y, 20, 20, MAROON);
//   DrawFPS(10, 10);

//   EndMode2D();
//   EndDrawing();
// }
