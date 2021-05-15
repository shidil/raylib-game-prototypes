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
#define BULLET_FIRE_RATE_MIN 20
#define BULLET_FIRE_RATE_MAX 20
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

enum WorldState {
  RUNNING,
  PAUSED,
  GAME_OVER,
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

typedef struct {
  Player player;
  std::vector<Enemy> enemies;
  std::vector<Bullet> bullets;
  WorldState state;
} GameWorld;

void draw_bullets(std::vector<Bullet> bullets, int count);
void draw_enemies(std::vector<Enemy> enemies, int count);
int update_bullets(std::vector<Bullet> &bullets, int count);
Bullet create_bullet(Enemy enemy, Player player);
Enemy create_enemy(int current_count);
Vector2 get_homing_velocity(Vector2 pos1, Vector2 pos2, int velocity);
GameWorld create_game_world();

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Dodge Machina");
  InitAudioDevice();

  // load resources
  Texture2D background = bomaqs::load_texture("bg-grid.png");
  Sound teleport_sfx = bomaqs::load_sound("teleport2.wav");
  Music bgm_music = bomaqs::load_music("n-Dimensions (Main Theme).mp3");
  bgm_music.looping = true;
  SetMusicVolume(bgm_music, 0.35f);

  PlayMusicStream(bgm_music);
  SetTargetFPS(60);

  unsigned long long int frames_count = 0;

  GameWorld game_world = create_game_world();

  while (!WindowShouldClose()) {
    // Update
    UpdateMusicStream(bgm_music);

    if (game_world.state == WorldState::RUNNING) {
      frames_count += 1;
    }

    if (GetGestureDetected() == GESTURE_TAP &&
        game_world.state == WorldState::GAME_OVER) {
      game_world = create_game_world();
      frames_count = 0;
    }

    int enemies_count = game_world.enemies.size();
    int bullets_count = game_world.bullets.size();

    // Player collisions with bullets
    for (int i = 0; i < bullets_count; i++) {
      if (CheckCollisionCircles(game_world.player.position, PLAYER_RADIUS,
                                game_world.bullets[i].position,
                                BULLET_RADIUS)) {
        game_world.player.state = ActorState::DEAD;
        game_world.state = WorldState::GAME_OVER;
      }
    }

    // Player collisions with enemies
    for (int i = 0; i < enemies_count; i++) {
      Rectangle enemy_rect = {
          .x = game_world.enemies[i].position.x,
          .y = game_world.enemies[i].position.y,
          .width = 20,
          .height = 20,
      };
      if (CheckCollisionCircleRec(game_world.player.position, PLAYER_RADIUS,
                                  enemy_rect)) {
        game_world.player.state = ActorState::DEAD;
        game_world.state = WorldState::GAME_OVER;
      }
    }

    // player input
    auto current_gesture = GetGestureDetected();
    if (game_world.player.state == LIVE && current_gesture == GESTURE_TAP) {
      auto touch_position = GetTouchPosition(0);

      PlaySoundMulti(teleport_sfx);
      game_world.player.position.x = touch_position.x;
      game_world.player.position.y = touch_position.y;
    }

    // Enemy collisions
    std::set<int> to_be_removed;
    for (int i = 0; i < enemies_count; i++) {
      Rectangle enemy_rect_1 = {
          .x = game_world.enemies[i].position.x,
          .y = game_world.enemies[i].position.y,
          .width = 20,
          .height = 20,
      };
      for (int j = i + 1; j < enemies_count; j++) {
        Rectangle enemy_rect_2 = {
            .x = game_world.enemies[j].position.x,
            .y = game_world.enemies[j].position.y,
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
      game_world.enemies.erase(game_world.enemies.begin() + idx);
      enemies_count -= 1;
    }

    if (game_world.state == WorldState::RUNNING) {
      // spawn enemy
      if (enemies_count < MAX_ENEMIES &&
          frames_count % (FRAME_RATE * (enemies_count ? 5 : 1)) == 0) {
        auto new_enemy = create_enemy(enemies_count);
        game_world.enemies.push_back(new_enemy);
        enemies_count++;
      }

      // update enemy
      for (int i = 0; i < enemies_count; i++) {
        Enemy *enemy = &game_world.enemies[i];

        switch (enemy->type) {
          case EnemyType::SHOOTER:
            if (frames_count == 0 || frames_count % enemy->fire_rate == 0) {
              if (bullets_count < MAX_BULLETS) {
                game_world.bullets.push_back(
                    create_bullet(*enemy, game_world.player));
                bullets_count += 1;
              }
            }
            break;
          case EnemyType::DASHER: {
            // skip enemy that is already dashing
            if (enemy->velocity.x == 0 && enemy->velocity.y == 0) {
              auto vel = get_homing_velocity(game_world.player.position,
                                             enemy->position, DASHER_VELOCITY);
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
          case EnemyType::HOMING: {
            auto vel = get_homing_velocity(game_world.player.position,
                                           enemy->position, HOMING_VELOCITY);
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
    bullets_count = update_bullets(game_world.bullets, bullets_count);

    //---- Draw
    BeginDrawing();
    ClearBackground(RAYWHITE);
    // DrawTexture(background, 0, 0, WHITE);

    // Draw game world
    DrawCircle(game_world.player.position.x, game_world.player.position.y,
               PLAYER_RADIUS, game_world.player.color);
    draw_enemies(game_world.enemies, enemies_count);
    draw_bullets(game_world.bullets, bullets_count);
    // debug dasher bounds
    DrawRectangleLinesEx(DASHER_BOUNDS, 2, LIGHTGRAY);

    // game over
    if (game_world.player.state == DEAD) {
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
  UnloadTexture(background);
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

GameWorld create_game_world() {
  Player player = {
      .position = {.x = SCREEN_WIDTH / 2, .y = SCREEN_HEIGHT - 200},
      .color = RED,
      .state = ActorState::LIVE};
  std::vector<Bullet> bullets;
  std::vector<Enemy> enemies;

  return {
      .player = player,
      .enemies = enemies,
      .bullets = bullets,
      .state = WorldState::RUNNING,
  };
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

Enemy create_enemy(int current_count) {
  // Avoid overlapping enemy and player, as well as other enemies
  // Keep min x distance from other enemies and player
  // Some randonmess in fire rate and other timings
  // Enemy spawn probability
  float x, y;
  EnemyType type;
  if (current_count == 0) {
    // First enemy is fixed
    x = (SCREEN_WIDTH / 2) + GetRandomValue(-100, 100);
    y = 100 + GetRandomValue(-25, 25);
    type = EnemyType::SHOOTER;
  } else {
    type = static_cast<EnemyType>(GetRandomValue(0, 2));

    // Spawn close to corners
    if (type == EnemyType::SHOOTER) {
      x = GetRandomValue(50, SCREEN_WIDTH - 50);
      y = GetRandomValue(50, SCREEN_HEIGHT - 50);
    } else {
      x = GetRandomValue(50, SCREEN_WIDTH - 50);
      y = GetRandomValue(50, SCREEN_HEIGHT - 50);
    }
  }

  Enemy enemy = {
      .position = {x, y},
      .color = enemy_colors[GetRandomValue(0, 2)],
      .velocity = {0, 0},
      .fire_rate = GetRandomValue(BULLET_FIRE_RATE_MIN, BULLET_FIRE_RATE_MAX),
      .type = type,
  };
  return enemy;
}

int update_bullets(std::vector<Bullet> &bullets, int count) {
  // TODO: bullets that go out of screen bounds needs to removed
  std::vector<int> to_be_erased;
  for (int i = 0; i < count; i++) {
    if (!CheckCollisionPointRec(bullets[i].position, BULLET_BOUNDS)) {
      to_be_erased.push_back(i);
    } else {
      bullets[i].position.x += bullets[i].velocity.x;
      bullets[i].position.y += bullets[i].velocity.y;
    }
  }
  for (auto idx : to_be_erased) {
    bullets.erase(bullets.begin() + idx);
  }

  return count - to_be_erased.size();
}

void draw_bullets(std::vector<Bullet> bullets, int count) {
  for (int i = 0; i < count; i++) {
    DrawCircle(bullets[i].position.x, bullets[i].position.y, BULLET_RADIUS,
               BLACK);
  }
}

void draw_enemies(std::vector<Enemy> enemies, int count) {
  for (int i = 0; i < count; i++) {
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
