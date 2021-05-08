cd build
conan install ..
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DGAME_ENTRY_FILE=$1
cmake --build .
