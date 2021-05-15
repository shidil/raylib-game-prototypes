# Build using emscripten
make PLATFORM=PLATFORM_WEB RAYLIB_PATH=$RAYLIB_HOME -B PROJECT_NAME=$1 $1

# Move build artifacts to public
mkdir -p public/$1
mv build/$1* public/$1
mv public/$1/$1.html public/$1/index.html

# Prepare manifest.json
cp src/manifest.json public/$1/manifest.json

# Replace {{title}} and {{game}} with values for this build
GAME_TITLE=$(echo $1 | awk -F"-" '{for(i=1;i<=NF;i++){$i=toupper(substr($i,1,1)) substr($i,2)}} 1' OFS=" ")
sed -i "s/{{title}}/$GAME_TITLE/" public/$1/manifest.json
sed -i "s/{{title}}/$GAME_TITLE/" public/$1/index.html
sed -i "s/{{game}}/$1/" public/$1/manifest.json
sed -i "s/{{game}}/$1/" public/$1/index.html

npx serve -s public/$1
