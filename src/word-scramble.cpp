#include <raylib.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

#include "utils/camera-2d.hpp"
#include "utils/data-loader.hpp"
#include "utils/math.hpp"

#define WINDOW_TITLE "Word Game"
#define SCREEN_WIDTH 360
#define SCREEN_HEIGHT 640
#define DEFAULT_FPS 60
#define GAME_SPEED 5
#define GAME_DIFFICULTY 3
#define GAME_OVER_MESSAGE "Failed!"
#define GAME_OVER_SCORE_TEXT "Your score: "

#define ANSWER_SIZE 24
#define LETTER_SIZE 48
#define REGULAR_SIZE 20

using namespace std;

typedef struct Letter {
  char value;
  int x;
  int y;
  Color color;
} Letter;

typedef struct Button {
  char *title;
  Rectangle bounds;
  Color color;
} Button;

typedef struct GameLevel {
  float timer;
  short button_order;
  short base_word_length;
  vector<Letter> letters;
  Button word1_button;
  Button word2_button;
} GameLevel;

enum Answer {
  CORRECT_ANSWER,
  WRONG_ANSWER,
  NO_ANSWER,
};

vector<Letter> generate_letters(char *, char *);
GameLevel generate_level(bomaqs::word_dict, short);
Answer check_answer(GameLevel, Vector2);
Color get_random_color(void);
char *get_random_word(bomaqs::word_dict, short);
void draw_level(GameLevel, Font);
void draw_game_over(int);
void draw_hud(GameLevel, int);

int main() {
  // Initialization
  auto word_dictionary = bomaqs::load_word_dictionary("word-list.txt");
  auto camera = bomaqs::create2dCamera();

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  InitAudioDevice();

  // Resources
  string basePath = ASSET_BASE_DIR;
  Font font = GetFontDefault();
  Sound wrong_answer_sfx = LoadSound("data/wrong.wav");
  Sound correct_answer_sfx = LoadSound("data/select.wav");
  Music music = LoadMusicStream("data/mini1111.ogg");

  PlayMusicStream(music);
  SetTargetFPS(DEFAULT_FPS);

  // Game world
  int score = 0;
  bool game_running = true;
  auto level = generate_level(word_dictionary, GAME_DIFFICULTY);

  // Input handling
  Vector2 touch_point = {0, 0};
  int current_gesture = GESTURE_NONE;
  int last_gesture = GESTURE_NONE;

  // Main game loop
  while (!WindowShouldClose()) {
    // Update
    UpdateMusicStream(music);
    level.timer -= GetFrameTime();

    // game over condition 1 timeout
    if (level.timer <= 0) {
      game_running = false;
    }

    // Restart game
    if (IsKeyDown(KEY_R)) {
      score = 0;
      game_running = true;
      level = generate_level(word_dictionary, GAME_DIFFICULTY);
    }

    // Check button press, correct answer go to next level, give score
    last_gesture = current_gesture;
    current_gesture = GetGestureDetected();
    touch_point = GetTouchPosition(0);

    if (current_gesture != last_gesture && current_gesture == GESTURE_TAP) {
      auto answer = check_answer(level, touch_point);

      if (game_running) {
        if (answer == CORRECT_ANSWER) {
          score += GAME_SPEED;
          level = generate_level(word_dictionary, GAME_DIFFICULTY);
          PlaySoundMulti(correct_answer_sfx);
        } else if (answer == WRONG_ANSWER) {
          game_running = false;
          PlaySoundMulti(wrong_answer_sfx);
        }
      }
    }

    // Draw
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    // Draw game world
    if (game_running) {
      draw_level(level, font);
      draw_hud(level, score);
    } else {
      draw_game_over(score);
    }

    EndMode2D();
    EndDrawing();
  }

  // De-Initialization
  StopSoundMulti();
  UnloadMusicStream(music);
  UnloadSound(correct_answer_sfx);
  UnloadSound(wrong_answer_sfx);
  CloseAudioDevice();
  CloseWindow();  // Close window and OpenGL context

  return 0;
}

GameLevel generate_level(bomaqs::word_dict word_dictionary, short difficulty) {
  short word1_length = GetRandomValue(difficulty, difficulty + 1);
  short word2_length =
      word1_length + GetRandomValue(word1_length == 3 ? 0 : -1, 1);

  char *word1 = get_random_word(word_dictionary, word1_length);
  char *word2 = get_random_word(word_dictionary, word2_length);

  vector<Letter> letters = generate_letters(word1, word2);

  Button word1_button = {.title = word1,
                         .bounds = (Rectangle){50, SCREEN_HEIGHT - 50,
                                               (SCREEN_WIDTH / 2 - 50), 50},
                         .color = get_random_color()};

  Button word2_button = {
      word2,
      (Rectangle){SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50, SCREEN_WIDTH / 2, 50},
      get_random_color()};

  return (GameLevel){
      .timer = GAME_SPEED,
      .button_order = (short)GetRandomValue(0, 1),
      .base_word_length = difficulty,
      .letters = generate_letters(word1, word2),
      .word1_button = word1_button,
      .word2_button = word2_button,
  };
}

void draw_level(GameLevel level, Font font) {
  // Draw letters
  int count_letters = level.letters.size();
  for (int i = 0; i < count_letters; i++) {
    auto letter = level.letters[i];
    char text[2] = {letter.value, '\0'};

    DrawText(text, letter.x, letter.y, LETTER_SIZE, letter.color);
  }

  // Correct answer is positioned randomly based on`button_order`
  if (level.button_order == 0) {
    DrawTextRec(font, level.word1_button.title, level.word1_button.bounds,
                ANSWER_SIZE, 12.0f, false, level.word1_button.color);
    DrawTextRec(font, level.word2_button.title, level.word2_button.bounds,
                ANSWER_SIZE, 12.0f, false, level.word2_button.color);
  } else {
    DrawTextRec(font, level.word1_button.title, level.word2_button.bounds,
                ANSWER_SIZE, 12.0f, false, level.word1_button.color);
    DrawTextRec(font, level.word2_button.title, level.word1_button.bounds,
                ANSWER_SIZE, 12.0f, false, level.word2_button.color);
  }
}

void draw_hud(GameLevel level, int score) {
  // Score
  string scoreText = "Score: ";
  DrawText(scoreText.append(to_string(score)).c_str(), 20, 10, REGULAR_SIZE,
           PURPLE);

  // Time remaining
  DrawText(to_string(bomaqs::to_fixed(level.timer, 2)).c_str(),
           SCREEN_WIDTH - 100, 10, REGULAR_SIZE, ORANGE);
}

Answer check_answer(GameLevel level, Vector2 touch_point) {
  // correct answer on left
  if (level.button_order == 0) {
    if (CheckCollisionPointRec(touch_point, level.word1_button.bounds)) {
      return CORRECT_ANSWER;
    }

    if (CheckCollisionPointRec(touch_point, level.word2_button.bounds)) {
      return WRONG_ANSWER;
    }
  }

  // correct answer on the right
  if (CheckCollisionPointRec(touch_point, level.word2_button.bounds)) {
    return CORRECT_ANSWER;
  }

  if (CheckCollisionPointRec(touch_point, level.word1_button.bounds)) {
    return WRONG_ANSWER;
  }

  return NO_ANSWER;
}

void draw_game_over(int score) {
  string scoreMessage = GAME_OVER_SCORE_TEXT;

  scoreMessage.append(to_string(score));
  DrawText(GAME_OVER_MESSAGE, 140, 280, REGULAR_SIZE, MAROON);
  DrawText(scoreMessage.c_str(), 100, 320, REGULAR_SIZE, ORANGE);
}

char *get_random_word(bomaqs::word_dict word_dictionary, short length) {
  // Get random word from list of words of given length
  auto wordList = word_dictionary[length];
  unsigned int index = rand() % (wordList.size() - 1);
  string word = (string)wordList[index];

  char *buffer = new char[length];
  strcpy(buffer, word.c_str());

  return buffer;
}

Color color_list[5] = {RED, MAROON, BLUE, VIOLET, DARKGRAY};

Color get_random_color() { return color_list[GetRandomValue(0, 4)]; }

vector<Letter> generate_letters(char *word1, char *word2) {
  string shuffled_word = word1;
  string modifier = word2;
  random_device rd;
  mt19937 g(rd());

  // Letters get shuffled
  shuffle(modifier.begin(), modifier.end(), g);
  shuffled_word.append(modifier.substr(0, 2));
  shuffle(shuffled_word.begin(), shuffled_word.end(), g);

  vector<Letter> letters = {};
  int row = 100;
  int per_column = GetRandomValue(2, 3);
  int column = 1;
  int count_letters = shuffled_word.length();
  for (int i = 0; i < count_letters; i++) {
    if (column == per_column) {
      row += 100;
      column = 1;
      per_column = GetRandomValue(2, 3);
    } else if (i > 0) {
      column += 1;
    }

    letters.push_back((Letter){
        .value = shuffled_word[i],
        .x = column * 80,
        .y = row,
        .color = get_random_color(),
    });
  }

  return letters;
}
