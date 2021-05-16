#include "dodge-machina.hpp"

#include <raylib.h>
#include <raymath.h>

#include <cmath>
#include <set>
#include <string>
#include <vector>

#include "utils/data-loader.hpp"
#include "utils/math.hpp"

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
  float score = 0;

  GameWorld game_world = create_game_world();

  // Main game loop runs FRAME_RATE times a second
  while (!WindowShouldClose()) {
    UpdateMusicStream(bgm_music);

    auto is_game_running = game_world.state == WorldState::RUNNING;
    auto is_game_over = game_world.state == WorldState::GAME_OVER;

    if (is_game_running) {
      frames_count += 1;
      score += 0.20f;
    }

    // Reset game on tap after game over screen shows
    if (GetGestureDetected() == GESTURE_TAP && is_game_over) {
      game_world = create_game_world();
      frames_count = 0;
      score = 0;
    }

    // Input handling
    auto current_gesture = GetGestureDetected();
    auto touch_position = GetTouchPosition(0);

    // Tapping anywhere will teleport player to that position
    if (game_world.player.state == LIVE && current_gesture == GESTURE_TAP) {
      // PlaySoundMulti(teleport_sfx);
      game_world.player.position.x = touch_position.x;
      game_world.player.position.y = touch_position.y;
    }

    // If player collides with bullet, shield loss for player
    if (check_bullet_collisions(game_world.player, game_world.bullets)) {
      game_world.player.shield -= 1;
    }

    // Check if player is caught in blast radius of a homer enemy
    if (check_homer_blast_collisions(game_world.player, game_world.enemies)) {
      game_world.player.shield -= 1;
    }

    // Player collisions with enemies
    auto collided_enemies = check_enemy_collisions(game_world.player, game_world.enemies);

    if (collided_enemies.size()) {
      for (auto idx : collided_enemies) {
        // If enemy is reloading, kill enemy, otherwise game over for player
        if (game_world.enemies[idx].state == ActorState::RELOADING) {
          game_world.enemies[idx].state = ActorState::DEAD;
        } else {
          game_world.player.shield -= 1;
        }
      }
    }

    // When player is out of shields and get hit, game over
    if (game_world.player.shield < 0 && game_world.player.state != ActorState::DEAD) {
      game_world.player.state = ActorState::DEAD;
      game_world.state = WorldState::GAME_OVER;
    }

    // Enemy-enemy collisions, both enemies die, player receives bonus score
    collided_enemies = check_enemy_enemy_collisions(game_world.enemies);

    if (collided_enemies.size()) {
      for (auto idx : collided_enemies) {
        game_world.enemies[idx].state = ActorState::DEAD;
        score += ENEMY_SELF_KILL_BONUS;
        // TODO: PlaySound(bonus_score_sfx);
      }
    }

    if (game_world.state == WorldState::RUNNING) {
      // spawn new enemy
      int enemies_count = game_world.enemies.size();
      if (enemies_count < MAX_ENEMIES &&
          frames_count % (FRAME_RATE * (enemies_count ? 5 : 1)) == 0) {
        game_world.enemies.push_back(create_enemy(enemies_count));
        enemies_count += 1;
      }

      // update enemy, shoot, dash, follow
      for (int i = 0; i < enemies_count; i++) {
        Enemy *enemy = &game_world.enemies[i];

        // Dead enemies can't shoot or dash
        if (enemy->state == ActorState::DEAD) {
          continue;
        }

        if (enemy->reload_timer >= 0) {
          enemy->reload_timer -= GetFrameTime();
          if (enemy->reload_timer <= 0) {
            // shooters and dashers gets back to their business
            if (enemy->state == ActorState::RELOADING) {
              enemy->state = ActorState::LIVE;
            }
            // explode homers
            if (enemy->state == ActorState::DESTRUCT) {
              enemy->state = ActorState::DEAD;
              // TODO: play explosion sound effect/trigger vfx
            }
          }
        }

        if (enemy->state == ActorState::RELOADING || enemy->state == ActorState::DESTRUCT) {
          continue;
        }

        switch (enemy->type) {
          case EnemyType::SHOOTER: {
            // enemy can shoot at set intervals (based on enemy.fire_rate)
            if (frames_count == 0 || frames_count % enemy->fire_rate == 0) {
              if (game_world.bullets.size() < MAX_BULLETS) {
                enemy->shots_fired += 1;
                game_world.bullets.push_back(create_bullet(*enemy, game_world.player));
              }
              // Set enemy state to RELOADING after x amount of bullets
              if (enemy->shots_fired >= enemy->shots_per_round) {
                enemy->shots_fired = 0;
                enemy->state = ActorState::RELOADING;
                enemy->reload_timer = ENEMY_RELOAD_TIMER;
              }
            }

            // Increase fire rate at set interval
            if (frames_count % FIRE_RATE_RAMPUP_INTERVAL == 0) {
              enemy->fire_rate = std::max(enemy->fire_rate - 1, BULLET_FIRE_RATE_MAX);
            }
            break;
          }
          case EnemyType::DASHER: {
            // TODO: tweak bound rect, may be check enemy rect center point
            // inside dasher bounds?
            Rectangle enemy_rect = {
                .x = enemy->position.x,
                .y = enemy->position.y,
                .width = 20,
                .height = 20,
            };

            // skip enemy that is already dashing
            if (enemy->velocity.x == 0 && enemy->velocity.y == 0) {
              auto vel =
                  get_homing_velocity(game_world.player.position, enemy->position, DASHER_VELOCITY);
              enemy->velocity.x = vel.x;
              enemy->velocity.y = vel.y;
            }
            // check dasher bounds, if inside continue to move, or stop moving
            else if (!CheckCollisionRecs(DASHER_BOUNDS, enemy_rect)) {
              enemy->velocity.x = 0;
              enemy->velocity.y = 0;
              enemy->state = ActorState::RELOADING;
              enemy->reload_timer = ENEMY_RELOAD_TIMER;
            }
            break;
          }
          case EnemyType::HOMING: {
            auto vel =
                get_homing_velocity(game_world.player.position, enemy->position, HOMING_VELOCITY);
            enemy->velocity.x = vel.x;
            enemy->velocity.y = vel.y;

            // If homer is at a set distance from player, trigger explosion with a set blast radius
            float distance = Vector2Distance(enemy->position, game_world.player.position);
            if (distance <= HOMER_BLAST_TRIGGER_DISTANCE) {
              enemy->state = ActorState::DESTRUCT;
              enemy->reload_timer = ENEMY_RELOAD_TIMER;
              enemy->trail_pos.clear();
            }
            break;
          }
          default:
            break;
        }

        // store enemy current pos to it's trail
        if (enemy->trail_pos.size() == MAX_ENEMY_TRAIL) {
          enemy->trail_pos.erase(enemy->trail_pos.begin());
        }
        enemy->trail_pos.push_back(enemy->position);

        // Moves enemy with respect to it's velocity and direction
        enemy->position.x += enemy->velocity.x;
        enemy->position.y += enemy->velocity.y;
      }
    }

    // Remove dead enemies
    std::vector<Enemy> updated_list;
    for (auto enemy : game_world.enemies) {
      // TODO: remove enemy after a delay
      if (enemy.state != ActorState::DEAD) {
        updated_list.push_back(enemy);
      }
    }
    game_world.enemies = updated_list;

    // bullets update, remove out of bound bullets
    game_world.bullets = update_bullets(game_world.bullets);

    //---- Draw
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexture(background, 0, 0, (Color){15, 15, 15, 255});

    // Draw game world
    DrawCircleLines(game_world.player.position.x, game_world.player.position.y, PLAYER_RADIUS,
                    game_world.player.color);
    draw_enemies(game_world.enemies);
    draw_bullets(game_world.bullets);
    // debug dasher bounds
    DrawRectangleLinesEx(DASHER_BOUNDS, 2, GREEN);

    // game over
    if (game_world.player.state == DEAD) {
      DrawText("You Died!", (SCREEN_WIDTH / 2) - 100, (SCREEN_HEIGHT / 2) - 25, 40, YELLOW);
    }

    DrawFPS(10, 10);
    std::string score_text = "Score: ";
    score_text.append(TextFormat("%02.00f", score));
    DrawText(score_text.data(), SCREEN_WIDTH - 120, 10, 20, ORANGE);

    std::string shield_string = "Shields: ";
    shield_string.append(std::to_string(std::max(0, game_world.player.shield)));
    DrawText(shield_string.data(), SCREEN_WIDTH / 2 - 50, 10, 20, GRAY);

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

Vector2 get_homing_velocity(Vector2 pos1, Vector2 pos2, int velocity) {
  auto angle = bomaqs::coordinate_angle(pos1, pos2);
  return {(float)cos(angle) * velocity, (float)sin(angle) * velocity};
}

GameWorld create_game_world() {
  Player player = {.position = {.x = SCREEN_WIDTH / 2, .y = SCREEN_HEIGHT - 200},
                   .color = RED,
                   .state = ActorState::LIVE,
                   .shield = INITIAL_PLAYER_SHIELDS};
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
      .velocity = get_homing_velocity(player.position, enemy.position, BULLET_VELOCITY),
  };

  return bullet;
}

Color enemy_colors[3] = {DARKGREEN, BLUE, VIOLET};
EnemyType enemy_order[MAX_ENEMIES] = {EnemyType::SHOOTER, EnemyType::DASHER, EnemyType::HOMING,
                                      EnemyType::DASHER};

Enemy create_enemy(int current_count) {
  // Avoid overlapping enemy and player, as well as other enemies
  // Keep min x distance from other enemies and player
  // Some randonmess in fire rate and other timings
  // Enemy spawn probability
  float x, y;
  EnemyType type = enemy_order[current_count % MAX_ENEMIES];
  if (current_count == 0) {
    // First enemy is fixed
    x = (SCREEN_WIDTH / 2) + GetRandomValue(-100, 100);
    y = 100 + GetRandomValue(-25, 25);
    // type = EnemyType::SHOOTER;
  } else {
    // type = static_cast<EnemyType>(GetRandomValue(0, 2));

    // Spawn shooters close to edges
    if (type == EnemyType::SHOOTER || type == EnemyType::DASHER) {
      auto left_align = GetRandomValue(0, 1);
      x = left_align ? 50 : SCREEN_WIDTH - 50;
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
      .type = type,
      .state = ActorState::LIVE,
      .fire_rate = BULLET_FIRE_RATE_MIN,
      .shots_fired = 0,
      .shots_per_round = RIFLE_SHOTS_PER_ROUND,
      .reload_timer = 0,
      .trail_pos = {},
  };
  return enemy;
}

std::vector<Bullet> update_bullets(std::vector<Bullet> &bullets) {
  std::vector<Bullet> updated;
  for (auto bullet : bullets) {
    if (CheckCollisionPointRec(bullet.position, BULLET_BOUNDS)) {
      bullet.position.x += bullet.velocity.x;
      bullet.position.y += bullet.velocity.y;
      updated.push_back(bullet);
    }
  }

  return updated;
}

bool check_bullet_collisions(Player player, std::vector<Bullet> bullets) {
  for (int i = 0; i < bullets.size(); i++) {
    if (CheckCollisionCircles(player.position, PLAYER_RADIUS, bullets[i].position, BULLET_RADIUS)) {
      return true;
    }
  }

  return false;
}

std::vector<int> check_enemy_collisions(Player player, std::vector<Enemy> enemies) {
  std::vector<int> out;
  for (int i = 0; i < enemies.size(); i++) {
    Rectangle enemy_rect = {
        .x = enemies[i].position.x,
        .y = enemies[i].position.y,
        .width = 20,
        .height = 20,
    };
    if (CheckCollisionCircleRec(player.position, PLAYER_RADIUS, enemy_rect)) {
      out.push_back(i);
    }
  }

  return out;
}

bool check_homer_blast_collisions(Player player, std::vector<Enemy> enemies) {
  for (int i = 0; i < enemies.size(); i++) {
    // skip non blast mode enemies. blast mode enemies will have state DESTRUCT
    if (enemies[i].state != ActorState::DESTRUCT) {
      continue;
    }

    // check if player hit box(circle) is colliding with blast/explosion circle
    if (CheckCollisionCircles(player.position, PLAYER_RADIUS, enemies[i].position,
                              HOMER_BLAST_RADIUS)) {
      return true;
    }
  }

  return false;
}

std::vector<int> check_enemy_enemy_collisions(std::vector<Enemy> enemies) {
  std::vector<int> out;
  int enemies_count = enemies.size();

  for (int i = 0; i < enemies_count; i++) {
    Rectangle enemy_rect_1 = {
        .x = enemies[i].position.x,
        .y = enemies[i].position.y,
        .width = 20,
        .height = 20,
    };
    for (int j = i + 1; j < enemies_count; j++) {
      Rectangle enemy_rect_2 = {
          .x = enemies[j].position.x,
          .y = enemies[j].position.y,
          .width = 20,
          .height = 20,
      };
      // if enemy i and j collides, they both die, bonus score!!
      if (CheckCollisionRecs(enemy_rect_1, enemy_rect_2)) {
        out.push_back(i);
        out.push_back(j);
      }
    }
  }

  return out;
}

void draw_bullets(std::vector<Bullet> bullets) {
  for (int i = 0; i < bullets.size(); i++) {
    DrawCircle(bullets[i].position.x, bullets[i].position.y, BULLET_RADIUS, YELLOW);
  }
}

void draw_enemies(std::vector<Enemy> enemies) {
  for (int i = 0; i < enemies.size(); i++) {
    auto enemy = enemies[i];
    Color color = enemy.color;
    if (enemy.state == ActorState::RELOADING) {
      color = GetRandomValue(0, 1) ? RED : color;
    }

    switch (enemy.type) {
      case EnemyType::HOMING:
        DrawRectangleLines(enemy.position.x, enemy.position.y, 20, 20, color);
        if (enemy.reload_timer > 0) {
          // draw a blast radius indicator as a circle based on current progress towars blast from
          // reload timer calculated as a percentage function
          auto blast_radi = (1 - (enemy.reload_timer / ENEMY_RELOAD_TIMER)) * HOMER_BLAST_RADIUS;
          DrawCircleLines(enemy.position.x, enemy.position.y, blast_radi, ORANGE);
        }
        break;
      case EnemyType::DASHER: {
        DrawRectangleLines(enemy.position.x, enemy.position.y, 20, 20, color);
        break;
      }
      default:
        DrawRectangleLines(enemy.position.x, enemy.position.y, 20, 20, color);
        break;
    }

    if (enemy.state == ActorState::LIVE && enemy.velocity.x != 0 && enemy.velocity.y != 0) {
      // Draw movement trail
      for (int i = MAX_ENEMY_TRAIL - 1; i >= 0; i -= 1) {
        auto trail_pos = enemy.trail_pos[i];
        color.a /= 2;
        DrawRectangleLines(trail_pos.x, trail_pos.y, 20 - MAX_ENEMY_TRAIL + i,
                           20 - MAX_ENEMY_TRAIL + i, color);
      }
    }
  }
}
