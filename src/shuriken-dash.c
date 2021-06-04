#include <raylib.h>

#define SCREEN_WIDTH 450
#define SCREEN_HEIGHT 800
#define WINDOW_TITLE "Shuriken Dash"
#define FRAME_RATE 60

#define PLATFORM_HEIGHT 250
#define PLAYER_HEIGHT 50
#define MAX_PLATFORMS 20
#define PLATFORM_WIDTH_MIN 100
#define PLATFORM_WIDTH_MAX 150
#define PLATFORM_GAP_MIN 50
#define PLATFORM_GAP_MAX 70
#define GRAVITY 98

#define SHURIKEN_SPEED 400

typedef Rectangle Platform;

typedef enum PlayerState {
  PLAYER_STATE_DEAD,
  PLAYER_STATE_DASHING,
  PLAYER_STATE_IDLE,
} PlayerState;

typedef struct {
  Vector2 position;
  PlayerState state;
} Player;

typedef struct GameWorld {
  Player player;
  Vector2 shuriken;
  Platform platforms[MAX_PLATFORMS];
} GameWorld;

GameWorld CreateWorld() {
  GameWorld world;

  // generate platforms
  for (int i = 0; i < MAX_PLATFORMS; i++) {
    int width = GetRandomValue(PLATFORM_WIDTH_MIN, PLATFORM_WIDTH_MAX);
    int gap = GetRandomValue(PLATFORM_GAP_MIN, PLATFORM_GAP_MAX);
    int x = 0, y = SCREEN_HEIGHT - PLATFORM_HEIGHT;

    if (i > 0) {
      Platform prev = world.platforms[i - 1];
      x = prev.x + prev.width + gap;
    }

    world.platforms[i] = (Platform){
        .x = x,
        .y = y,
        .width = width,
        .height = PLATFORM_HEIGHT,
    };
  }

  Platform firstPlatform = world.platforms[0];
  int playerX = firstPlatform.x + (firstPlatform.width / 2) - (PLAYER_HEIGHT / 2);
  int playerY = SCREEN_HEIGHT - firstPlatform.height - (PLAYER_HEIGHT * 1.5);

  world.player = (Player){{playerX, playerY}, PLAYER_STATE_IDLE};
  world.shuriken = world.player.position;

  return world;
}

int main(void) {
  // initialization
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);

  // resources

  SetTargetFPS(FRAME_RATE);

  // Game variables
  GameWorld world = CreateWorld();
  int lastGesture = GESTURE_NONE;
  int currentGesture = GESTURE_NONE;
  Vector2 touchPosition = {0, 0};
  int throwDistance = 0;

  // Camera
  Camera2D camera = {0};
  camera.target = (Vector2){world.shuriken.x + 20.0f, world.shuriken.y + 20.0f};
  camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  // Game loop
  while (!WindowShouldClose()) {
    float delta = GetFrameTime();

    // Update & Input
    lastGesture = currentGesture;
    currentGesture = GetGestureDetected();
    touchPosition = GetTouchPosition(0);

    // Restart game on R
    if (GetKeyPressed() == KEY_R) {
      world = CreateWorld();
      throwDistance = 0;
      lastGesture = GESTURE_NONE;
      currentGesture = GESTURE_NONE;
    }

    // Camera target follows player
    camera.target = (Vector2){world.shuriken.x + 20, world.shuriken.y + 20};

    // set shuriken throw distance on tap and hold
    if (world.player.state == PLAYER_STATE_IDLE && currentGesture == GESTURE_HOLD) {
      throwDistance += (SHURIKEN_SPEED * delta);
    }

    // on release tap and hold
    if (lastGesture == GESTURE_HOLD && currentGesture != lastGesture) {
      if (throwDistance < 25) {
        throwDistance = 0;
      } else {
        world.player.state = PLAYER_STATE_DASHING;
      }
    }

    // Move shuriken forward
    if (world.player.state == PLAYER_STATE_DASHING && throwDistance > 0) {
      float displacement = (SHURIKEN_SPEED * delta);
      world.shuriken.x += displacement;
      throwDistance -= displacement;
    }

    // Teleport player after shuriken reaches throw distance
    if (world.player.state == PLAYER_STATE_DASHING && throwDistance <= 0) {
      world.player.state = PLAYER_STATE_IDLE;
      world.player.position = world.shuriken;
      throwDistance = 0;
    }

    // Player gravity and collision
    Rectangle playerRect = {.x = world.player.position.x - (PLAYER_HEIGHT / 4),
                            .y = world.player.position.y - (PLAYER_HEIGHT / 2),
                            .width = PLAYER_HEIGHT / 4,
                            .height = PLAYER_HEIGHT};
    int playerOnPlatform = false;
    for (int i = 0; i < MAX_PLATFORMS; i++) {
      if (CheckCollisionRecs(playerRect, world.platforms[i])) {
        playerOnPlatform = true;
        break;
      }
    }

    if (!playerOnPlatform && world.player.state == PLAYER_STATE_IDLE) {
      world.player.position.y += GRAVITY * delta;
    }

    if (world.player.position.y > (SCREEN_HEIGHT - 75)) {
      world.player.state = PLAYER_STATE_DEAD;
    }

    // Draw
    BeginDrawing();
    ClearBackground(WHITE);
    BeginMode2D(camera);

    // Draw platforms
    for (int i = 0; i < MAX_PLATFORMS; i++) {
      DrawRectangleRec(world.platforms[i], MAROON);
    }

    // Draw shuriken
    if (world.player.state == PLAYER_STATE_DASHING) {
      DrawCircle(world.shuriken.x, world.shuriken.y, 10, BLACK);
    }

    // Draw Player
    Vector2 playerPos = world.player.position;
    DrawRectangle(playerPos.x, playerPos.y, PLAYER_HEIGHT / 2, PLAYER_HEIGHT, DARKBLUE);

    EndMode2D();

    if (world.player.state == PLAYER_STATE_DEAD) {
      DrawText("You Died", SCREEN_WIDTH / 2 - 75, SCREEN_HEIGHT / 2 - 25, 25, ORANGE);
    }

    DrawFPS(10, 10);
    EndDrawing();
  }

  // Unload and terminate
  CloseWindow();
  return 0;
}
