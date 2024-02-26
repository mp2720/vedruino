SRC=$(shell find src/ -type f -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.hpp' -o -name '*.h' -o -name '*.S')

CONF=./config.ini

SKETCH=$(shell tools/config.py ${CONF} -g tools:sketch_name)
SERIAL_PORT=$(shell tools/config.py ${CONF} -g tools:serial_port)
FQBN=$(shell tools/config.py ${CONF} -g tools:fqbn)
BOARD_HOSTNAME=$(shell tools/config.py ${CONF} -g tools:board_hostname)
NETLOG_FLAGS=$(shell tools/config.py ${CONF} -g tools:netlog_flags)
SERIAL_FLAGS=$(shell tools/config.py ${CONF} -g tools:serial_flags)
TOOLCHAIN_PATH=$(shell tools/config.py ${CONF} -g tools:xtensa_toolchain_path)

BIN_PATH=build/${SKETCH}.ino.bin

src/conf.h: config.ini
	tools/config.py ${CONF} -c
 
build/${SKETCH}.ino.bin: $(SRC) src/conf.h
	arduino-cli compile --build-path ./build

.PHONY: conf clean cleanall setup build updcc flash mon ota netlog disasm all

conf: src/conf.h

clean:
	rm -f ${BIN_PATH}

cleanall:
	rm -rf build src/conf.h

setup:
	arduino-cli board attach -p ${SERIAL_PORT} -b ${FQBN}

build: ${BIN_PATH} conf

updcc:
	arduino-cli compile --only-compilation-database --build-path ./build
	tools/compile_commands.py < build/compile_commands.json > compile_commands.json

flash: build
	arduino-cli upload -b ${FQBN} --input-dir ./build

mon:
	tools/mon.py ${SERIAL_FLAGS} /dev/ttyUSB0 | tools/btrace.py ${TOOLCHAIN_PATH}/xtensa-esp32-elf-addr2line build/${SKETCH}.ino.elf

ota: build
	tools/ota.py ${BIN_PATH} ${BOARD_HOSTNAME}

netlog:
	tools/netlog.py ${NETLOG_FLAGS} ${BOARD_HOSTNAME} | tools/btrace.py $(TOOLCHAIN_PATH)/xtensa-esp32-elf-addr2line build/${SKETCH}

disasm:
	${TOOLCHAIN_PATH}/xtensa-esp32-elf-objdump -d build/${SKETCH}.ino.elf > /tmp/${SKETCH}.s

all: build flash mon

