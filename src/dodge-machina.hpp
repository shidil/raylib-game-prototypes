#include <raylib.h>

#include <vector>

#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 960
#define FRAME_RATE 60

#define PLAYER_RADIUS 20
#define INITIAL_PLAYER_SHIELDS 3

#define ENEMY_KILL_BONUS 50
#define ENEMY_SELF_KILL_BONUS 100

#define BULLET_RADIUS 3
#define BULLET_FIRE_RATE_MIN 20
#define BULLET_FIRE_RATE_MAX 10
#define MAX_BULLETS 100
#define RIFLE_SHOTS_PER_ROUND 25
#define BAZOOKA_SHOTS_PER_ROUND 1
#define BULLET_VELOCITY 5
#define FIRE_RATE_RAMPUP_INTERVAL 300

#define MAX_ENEMIES 4
#define MAX_ENEMY_TRAIL 10

#define DASHER_VELOCITY 8
#define HOMING_VELOCITY 2
#define ENEMY_RELOAD_TIMER 1.5f

#define HOMER_BLAST_RADIUS 60
#define HOMER_BLAST_TRIGGER_DISTANCE 50

#define DASHER_BOUNDS \
  CLITERAL(Rectangle) { 50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100 }
#define BULLET_BOUNDS \
  CLITERAL(Rectangle) { -50, -50, SCREEN_WIDTH + 100, SCREEN_HEIGHT + 100 }


enum EnemyType {
  SHOOTER,
  DASHER,
  HOMING,
};

enum ActorState {
  LIVE,
  DEAD,
  RELOADING,
  DESTRUCT,
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
  ActorState state;
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
  std::vector<Vector2> trail_pos;
} Enemy;

typedef struct {
  Vector2 position;
  Color color;
  ActorState state;
  int shield;
} Player;

typedef struct {
  Player player;
  std::vector<Enemy> enemies;
  std::vector<Bullet> bullets;
  WorldState state;
} GameWorld;

Vector2 get_homing_velocity(Vector2 pos1, Vector2 pos2, int velocity);

GameWorld create_game_world();

std::vector<Bullet> update_bullets(std::vector<Bullet> &bullets);
Bullet create_bullet(Enemy enemy, Player player);
void draw_bullets(std::vector<Bullet> bullets);

Enemy create_enemy(int current_count);
void draw_enemies(std::vector<Enemy> enemies);

bool check_bullet_collisions(Player player, std::vector<Bullet> &bullets);
std::vector<int> check_enemy_collisions(Player player, std::vector<Enemy> enemies);
std::vector<int> check_enemy_enemy_collisions(std::vector<Enemy> enemies);
bool check_homer_blast_collisions(Player player, std::vector<Enemy> enemies);
