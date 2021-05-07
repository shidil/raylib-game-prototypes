#include <fstream>
#include <nlohmann/json.hpp>
#define ASSET_BASE_DIR "data/";

using json = nlohmann::json;
using ifstream = std::ifstream;

namespace bomaqs {

json load_from_json(std::string jsonFilePath) {
  json data;
  std::string path = ASSET_BASE_DIR;
  ifstream file(path.append(jsonFilePath));

  file >> data;
  return data;
}

}  // namespace bomaqs
