#include <raylib.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

#include "utils/camera-2d.hpp"
#include "utils/data-loader.hpp"

#define WINDOW_TITLE "Word Game"
#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 960
#define DEFAULT_FPS 60
#define GAME_SPEED 3
#define GAME_DIFFICULTY 3
#define GAME_OVER_TIMEOUT_MESSAGE "Time out!"
#define GAME_OVER_INCORRECT_MESSAGE "Wrong answer!"
#define GAME_OVER_SCORE_TEXT "Your score: "

#define ANSWER_SIZE 42
#define LETTER_SIZE 80
#define REGULAR_SIZE 28

#define ALL_ALPHABETS "abcdefghijklmnopqrstuvwxyz"

using namespace std;

enum Answer {
  CORRECT_ANSWER,
  WRONG_ANSWER,
  NO_ANSWER,
};

enum LevelAnswer {
  LEFT_WORD,
  RIGHT_WORD,
  BOTH_WORDS,
};

typedef short Chance;

typedef struct Letter {
  char value;
  float x;
  float y;
  Color color;
} Letter;

typedef struct Button {
  char* title;
  Rectangle bounds;
  Color color;
} Button;

typedef struct GameLevel {
  float timer;
  short base_word_length;
  vector<Letter> letters;
  Button word1_button;
  Button word2_button;
  LevelAnswer answer;
} GameLevel;

vector<Letter> generate_letters(string, string);
GameLevel generate_level(bomaqs::word_dict, short);
Answer check_answer(GameLevel, Vector2);
Color get_random_color(void);
char* get_random_word(bomaqs::word_dict, short);
void draw_level(GameLevel, Font, Font);
void draw_game_over(int, string);
void draw_background(Texture2D);
void draw_hud(GameLevel, int);

int main() {
  //---- Initialization
  auto camera = bomaqs::create2dCamera();

  // SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  InitAudioDevice();

  // Resources
  auto word_dictionary = bomaqs::load_word_dictionary("word-list.txt");
  // Texture2D background = bomaqs::load_texture("bg1-original.png");
  Font letter_font = bomaqs::load_font("Cousine-Regular.ttf", LETTER_SIZE);
  Font button_font = bomaqs::load_font("IBMPlexMono-Regular.ttf", ANSWER_SIZE);
  Sound wrong_answer_sfx = bomaqs::load_sound("wrong.wav");
  Sound correct_answer_sfx = bomaqs::load_sound("select.wav");
  Music music = bomaqs::load_music("mini1111.ogg");

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

  //---- Main game loop
  while (!WindowShouldClose()) {
    //---- Update
    UpdateMusicStream(music);

    if (game_running) {
      level.timer -= GetFrameTime();
    }

    // game over condition 1 timeout
    if (level.timer <= 0) {
      game_running = false;
    }

    // Check button press, correct answer go to next level, give score
    last_gesture = current_gesture;
    current_gesture = GetGestureDetected();
    touch_point = GetTouchPosition(0);

    if (current_gesture != last_gesture && current_gesture == GESTURE_TAP) {
      if (game_running) {
        switch (check_answer(level, touch_point)) {
          case CORRECT_ANSWER:
            score += GAME_SPEED;
            level = generate_level(word_dictionary, GAME_DIFFICULTY);
            PlaySoundMulti(correct_answer_sfx);
            break;
          case WRONG_ANSWER:
            game_running = false;
            PlaySoundMulti(wrong_answer_sfx);
            break;
          default:
            break;
        }
      } else {
        score = 0;
        game_running = true;
        level = generate_level(word_dictionary, GAME_DIFFICULTY);
      }
    }

    //---- Draw
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    // Draw game world
    if (game_running) {
      // draw_background(background);
      draw_hud(level, score);
      draw_level(level, letter_font, button_font);
    } else {
      auto message = level.timer <= 0 ? GAME_OVER_TIMEOUT_MESSAGE
                                      : GAME_OVER_INCORRECT_MESSAGE;
      draw_game_over(score, message);
    }

    EndMode2D();
    EndDrawing();
  }

  //---- De-Initialization
  StopSoundMulti();
  UnloadMusicStream(music);
  UnloadSound(correct_answer_sfx);
  UnloadSound(wrong_answer_sfx);
  CloseAudioDevice();
  CloseWindow();

  return 0;
}

GameLevel generate_level(bomaqs::word_dict word_dictionary, short difficulty) {
  short word1_length = GetRandomValue(difficulty, difficulty + 1);
  short word2_length =
      word1_length + GetRandomValue(word1_length == 3 ? 0 : -1, 1);

  char* word1 = get_random_word(word_dictionary, word1_length);
  char* word2 = get_random_word(word_dictionary, word2_length);

  vector<Letter> letters = generate_letters(word1, word2);

  Button word1_button = {
      .title = word1,
      .bounds = (Rectangle){0, SCREEN_HEIGHT - 120, (SCREEN_WIDTH / 2 - 15), 80},
      .color = get_random_color()};

  Button word2_button = {
      word2,
      (Rectangle){SCREEN_WIDTH / 2, SCREEN_HEIGHT - 120, SCREEN_WIDTH / 2, 80},
      get_random_color()};

  auto answer_order = static_cast<LevelAnswer>(GetRandomValue(0, 2));
  string answer = answer_order == RIGHT_WORD ? word2 : word1;
  string seed = answer_order == BOTH_WORDS ? word2 : "";

  if (answer_order != BOTH_WORDS) {
    string all_alphabets = ALL_ALPHABETS;
    string wrong_answer = answer_order == RIGHT_WORD ? word1 : word2;

    // If there is only one correct answer, set seed to all alphabets except
    // ones present in the wrong answer
    for (int i = 0; i < all_alphabets.size(); i++) {
      if (wrong_answer.find(all_alphabets[i]) == -1) {
        seed.push_back(all_alphabets[i]);
      }
    }
  }

  return (GameLevel){
      .timer = GAME_SPEED,
      .base_word_length = difficulty,
      .letters = generate_letters(answer, seed),
      .word1_button = word1_button,
      .word2_button = word2_button,
      .answer = answer_order,
  };
}

void draw_background(Texture2D background) {
  DrawTextureEx(background, (Vector2){0, 0}, 0.0f, 0.5f, WHITE);
}

void draw_level(GameLevel level, Font letter_font, Font button_font) {
  // Draw letters
  int count_letters = level.letters.size();
  for (int i = 0; i < count_letters; i++) {
    auto letter = level.letters[i];
    char text[2] = {letter.value, '\0'};

    DrawTextEx(letter_font, text, (Vector2){letter.x, letter.y}, LETTER_SIZE, 0,
               letter.color);
  }

  Rectangle bound_left = level.word1_button.bounds;
  Rectangle bound_right = level.word2_button.bounds;

  // Correct answer is positioned randomly based on`button_order`
  DrawTextRec(
      button_font, level.word1_button.title,
      (Rectangle){bound_left.x + 80, bound_left.y + bound_left.height / 4,
                  bound_left.width, bound_left.height},
      ANSWER_SIZE, 8.0f, false, level.word1_button.color);
  DrawTextRec(
      button_font, level.word2_button.title,
      (Rectangle){bound_right.x + 80, bound_right.y + bound_right.height / 4,
                  bound_right.width, bound_right.height},
      ANSWER_SIZE, 8.0f, false, level.word2_button.color);

  DrawRectangleLinesEx(bound_left, 1, LIGHTGRAY);
  DrawRectangleLinesEx(bound_right, 1, LIGHTGRAY);
}

void draw_hud(GameLevel level, int score) {
  // Score
  string scoreText = "Score: ";
  DrawText(scoreText.append(to_string(score)).c_str(), 20, 10, REGULAR_SIZE,
           GRAY);

  // Time remaining
  DrawText(TextFormat("%02.02f", level.timer), SCREEN_WIDTH - 60, 10,
           REGULAR_SIZE, ORANGE);
}

Answer check_answer(GameLevel level, Vector2 touch_point) {
  if (CheckCollisionPointRec(touch_point, level.word1_button.bounds)) {
    return (level.answer != RIGHT_WORD) ? CORRECT_ANSWER : WRONG_ANSWER;
  }

  if (CheckCollisionPointRec(touch_point, level.word2_button.bounds)) {
    return (level.answer != LEFT_WORD) ? CORRECT_ANSWER : WRONG_ANSWER;
  }

  return NO_ANSWER;
}

void draw_game_over(int score, string message) {
  string scoreMessage = GAME_OVER_SCORE_TEXT;

  scoreMessage.append(to_string(score));
  DrawText(message.data(), 200, 380, REGULAR_SIZE, MAROON);
  DrawText(scoreMessage.data(), 180, 420, REGULAR_SIZE, ORANGE);
}

char* get_random_word(bomaqs::word_dict word_dictionary, short length) {
  // Get random word from list of words of given length
  auto wordList = word_dictionary[length];
  unsigned int index = rand() % (wordList.size() - 1);
  string word = (string)wordList[index];

  char* buffer = new char[length];
  strcpy(buffer, word.c_str());

  return buffer;
}

Color color_list[10] = {RED,       MAROON,   BLUE,  VIOLET, DARKGRAY,
                        DARKGREEN, DARKBLUE, BLACK, PURPLE, MAGENTA};

Color get_random_color() { return color_list[GetRandomValue(0, 9)]; }

vector<Letter> generate_letters(string answer, string seed) {
  string shuffled_word = answer;
  string modifier = seed;

  // Letters get shuffled
  random_device rd;
  mt19937 g(rd());
  shuffle(modifier.begin(), modifier.end(), g);
  shuffled_word.append(modifier.substr(0, 3));
  shuffle(shuffled_word.begin(), shuffled_word.end(), g);

  vector<Letter> letters = {};
  int row = 160;
  int per_column = GetRandomValue(2, 3);
  int column = 1;
  int count_letters = shuffled_word.length();
  for (int i = 0; i < count_letters; i++) {
    if (column == per_column) {
      row += 160;
      column = 1;
      per_column = GetRandomValue(2, 3);
    } else if (i > 0) {
      column += 1;
    }

    letters.push_back((Letter){
        .value = shuffled_word[i],
        .x = (float)column * 120,
        .y = (float)row,
        .color = get_random_color(),
    });
  }

  return letters;
}
