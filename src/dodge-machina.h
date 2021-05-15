#include <raylib.h>

#include <vector>

#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 960
#define FRAME_RATE 60
#define PLAYER_RADIUS 20
#define BULLET_RADIUS 5
#define BULLET_FIRE_RATE_MIN 20
#define BULLET_FIRE_RATE_MAX 8
#define MAX_BULLETS 100
#define RIFLE_SHOTS_PER_ROUND 40
#define BAZOOKA_SHOTS_PER_ROUND 1
#define BULLET_VELOCITY 10
#define MAX_ENEMIES 4
#define DASHER_VELOCITY 8
#define HOMING_VELOCITY 3
#define ENEMY_RELOAD_TIMER 5
#define DASHER_BOUNDS \
  CLITERAL(Rectangle) { 50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100 }
#define BULLET_BOUNDS \
  CLITERAL(Rectangle) { -50, -50, SCREEN_WIDTH + 100, SCREEN_HEIGHT + 100 }

#define ENEMY_SELF_KILL_BONUS 50
#define FIRE_RATE_RAMPUP_INTERVAL 500

enum EnemyType {
  SHOOTER,
  DASHER,
  HOMING,
};

enum ActorState {
  LIVE,
  DEAD,
  RELOADING,
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
  EnemyType type;
  ActorState state;
  int fire_rate;
  int shots_fired;
  int shots_per_round;
  float reload_timer;
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
