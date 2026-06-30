PIO = platformio

.PHONY: all build upload monitor clean rebuild

all: build

build:
	$(PIO) run

upload:
	$(PIO) run --target upload

monitor:
	$(PIO) device monitor

clean:
	$(PIO) run --target clean

rebuild: clean build
