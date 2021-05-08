#include <raylib.h>

#include <fstream>
#include <iostream>
#include <map>

#define ASSET_BASE_DIR "./resources/";

namespace bomaqs {

typedef std::vector<std::string> word_list;
typedef std::map<int, word_list> word_dict;

std::map<int, word_list> load_word_dictionary(std::string file_path) {
  std::map<int, word_list> dictionary;
  std::string path = ASSET_BASE_DIR;
  std::string contents = LoadFileText(path.append(file_path).c_str());

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
