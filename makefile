include config

.PHONY: setup build flash monitor all

setup:
	arduino-cli board attach -p ${PORT} -b ${FQBN}

updcc:
	acdb
	python comp_coms_halalizer.py

build:
	arduino-cli compile

flash:
	arduino-cli upload

monitor:
	arduino-cli monitor -p ${PORT}

all: build flash monitor

