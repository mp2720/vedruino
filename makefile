SRC=$(shell find src/ -type f -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.hpp' -o -name '*.h')

REPONAME=$(shell python tools/conf.py -g common.repo_name)
PORT=$(shell python tools/conf.py -g arduino.port)
FQBN=$(shell python tools/conf.py -g arduino.fqbn)
BAUD=$(shell python tools/conf.py -g arduino.baud)

src/conf.h: conf/config.ini
	tools/conf.py -c
 
build/${REPONAME}.ino.bin: $(SRC) conf
	arduino-cli compile --build-path ./build

.PHONY: clean cleanall setup build updcc flash monitor all

conf: src/conf.h

clean:
	rm -f build/${REPONAME}.ino.bin

cleanall:
	rm -rf build src/conf.h

setup:
	arduino-cli board attach -p ${PORT} -b ${FQBN}

build: build/${REPONAME}.ino.bin

updcc:
	arduino-cli compile --only-compilation-database --build-path ./build
	tools/compile_commands.py < build/compile_commands.json > compile_commands.json

flash: build
	arduino-cli upload -b ${FQBN} --input-dir ./build

monitor:
	arduino-cli monitor -p ${PORT} --config baudrate=${BAUD}
	clear

all: build flash monitor

