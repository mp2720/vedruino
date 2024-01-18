include config

.PHONY: setup build flash monitor all

setup:
	arduino-cli board attach -p ${PORT} -b ${FQBN}

build:
	arduino-cli compile

flash:
	arduino-cli upload

monitor:
	arduino-cli monitor -p ${PORT}

all: build flash monitor

