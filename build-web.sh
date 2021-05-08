make PLATFORM=PLATFORM_WEB RAYLIB_PATH=$RAYLIB_HOME -B PROJECT_NAME=$1 $1
mkdir -p public/$1
mv build/$1* public/$1
mv public/$1/$1.html public/$1/index.html
npx serve -s public/$1
