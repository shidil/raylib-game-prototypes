#include <raylib.h>

#include <cstring>
#include <iostream>
#include <map>

#define ASSET_BASE_DIR "resources/";

namespace bomaqs {

typedef std::vector<std::string> word_list;
typedef std::map<int, word_list> word_dict;

std::string get_real_path(std::string path) {
  std::string out = ASSET_BASE_DIR;
  return out.append(path);
}

std::string load_text_file(std::string file) {
  std::string contents = LoadFileText(get_real_path(file).data());
  return contents;
}

Sound load_sound(std::string file) {
  return LoadSound(get_real_path(file).data());
}

Music load_music(std::string file) {
  return LoadMusicStream(get_real_path(file).data());
}

Texture2D load_texture(std::string file) {
  return LoadTexture(get_real_path(file).data());
}

Font load_sdf_font(std::string font_name, int base_size = 16,
                   int char_count = 95) {
  // Loading file to memory
  unsigned int fileSize = 0;
  unsigned char* fileData =
      LoadFileData(get_real_path(font_name).data(), &fileSize);

  // SDF font generation from TTF font
  Font fontSDF = {0};
  fontSDF.baseSize = base_size;
  fontSDF.charsCount = char_count;

  // Parameters > font size: 16, no chars array provided (0), chars count: 0
  // (defaults to 95)
  fontSDF.chars = LoadFontData(fileData, fileSize, base_size, 0, 0, FONT_SDF);
  // Parameters > chars count: 95, font size: 16, chars padding in image: 0 px,
  // pack method: 1 (Skyline algorithm)
  Image atlas = GenImageFontAtlas(fontSDF.chars, &fontSDF.recs, char_count,
                                  base_size, 0, 1);
  fontSDF.texture = LoadTextureFromImage(atlas);
  UnloadImage(atlas);

  UnloadFileData(fileData);  // Free memory from loaded file

  return fontSDF;
}

Font load_font(std::string font_name, int base_size = 16, int char_count = 95) {
  // Loading file to memory
  unsigned int fileSize = 0;
  unsigned char* fileData =
      LoadFileData(get_real_path(font_name).data(), &fileSize);

  // SDF font generation from TTF font
  Font font = {0};
  font.baseSize = base_size;
  font.charsCount = char_count;

  // Parameters > font size: 16, no chars array provided (0), chars count: 0
  // (defaults to 95)
  font.chars =
      LoadFontData(fileData, fileSize, base_size, 0, char_count, FONT_DEFAULT);
  // Parameters > chars count: 95, font size: 16, chars padding in image: 0 px,
  // pack method: 1 (Skyline algorithm)
  Image atlas =
      GenImageFontAtlas(font.chars, &font.recs, char_count, base_size, 4, 0);
  font.texture = LoadTextureFromImage(atlas);
  UnloadImage(atlas);

  UnloadFileData(fileData);  // Free memory from loaded file

  return font;
}

std::map<int, word_list> load_word_dictionary(std::string file) {
  std::map<int, word_list> dictionary;
  std::string contents = load_text_file(file);

  int start = 0;
  int end = contents.find('\n');
  while (end != -1) {
    std::string word = contents.substr(start, end - start);
    start = end + 1;
    end = contents.find('\n', start);

    dictionary[word.size()].push_back(word);
  }

  return dictionary;
}
}  // namespace bomaqs
