SRC=$(shell find src/ -type f -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.hpp' -o -name '*.h')

SKETCH=$(shell tools/config.py -g arduino:sketch_name)
PORT=$(shell tools/config.py -g board:port)
FQBN=$(shell tools/config.py -g board:fqbn)
BAUD=$(shell tools/config.py -g board:baud)
BOARD_IP=$(shell tools/config.py -g board:ip)
OTA_PORT=$(shell tools/config.py -g tcp_ota:port)

BIN_PATH=build/${SKETCH}.ino.bin

ARDUINO_COMPILE_FLAGS=--build-properties build.partitions=min_spiffs,upload.maximum_size=1966080,compiler.c.extra_flags=-g

TOOLCHAIN_PATH=$(shell tools/config.py -g arduino:toolchain_path)

src/conf.h: config.ini
	tools/config.py -c
 
build/${SKETCH}.ino.bin: $(SRC)
	arduino-cli compile ${ARDUINO_COMPILE_FLAGS} --build-path ./build

.PHONY: conf clean cleanall setup build updcc flash monitor ota all

conf: src/conf.h

clean:
	rm -f ${BIN_PATH}

cleanall:
	rm -rf build src/conf.h

setup:
	arduino-cli board attach -p ${PORT} -b ${FQBN}

build: ${BIN_PATH} conf

updcc:
	arduino-cli compile --only-compilation-database --build-path ./build
	tools/compile_commands.py < build/compile_commands.json > compile_commands.json

flash: build
	arduino-cli upload -b ${FQBN} --input-dir ./build

monitor:
	arduino-cli monitor -p ${PORT} --config baudrate=${BAUD}
	clear

ota: build
	tools/ota.py -i ${BOARD_IP} -p ${OTA_PORT} ${BIN_PATH}

disasm:
	${TOOLCHAIN_PATH}/xtensa-esp32-elf-objdump -d build/${SKETCH}.ino.elf > /tmp/${SKETCH}.s


all: build flash monitor

