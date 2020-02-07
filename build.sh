#!/bin/sh

set -eu

./sokol-shdc --input demo.glsl --output demo.glsl.h --slang metal_macos:glsl100

mkdir -p ./dist

if [ `command -v clang-format -h` ]; then
  clang-format demo.c > demo.c.tmp
  mv demo.c.tmp demo.c
fi

gcc demo.c watt_math.c watt_buffer.c watt_input.c \
	-DSOKOL_METAL=1 \
	-o ./dist/demo \
	-ObjC \
	-fobjc-arc \
	-framework Cocoa \
	-framework QuartzCore \
	-framework Metal \
	-framework MetalKit \
	-framework AudioToolbox

emcc demo.c watt_math.c watt_buffer.c watt_input.c \
	-DSOKOL_GLES2=1 \
	-o ./dist/demo.js \
  --preload-file assets/toob.gltf \
  --preload-file assets/plus.gltf \
  --preload-file assets/reggie.gltf \
  --preload-file assets/rolo.gltf

cp ./index.html ./dist
