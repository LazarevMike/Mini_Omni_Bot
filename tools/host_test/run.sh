#!/usr/bin/env bash
# Runs the robot program on this computer against a mock Arduino core with simulated
# motors and encoders. Four runs of the whole program:
#   - ESP32 Arduino core 2.x API, phone joystick on (the default Romeo build)
#   - ESP32 Arduino core 3.x API, phone joystick on
#   - phone joystick switched off (REMOTE_CONTROL 0)
#   - a board that is not an ESP32: the phone joystick is left out automatically
# Then three runs of drivers_test.cpp, one per kind of motor driver (MOTOR_DRIVER): what
# goes on the driver pins, and that a stop stops the motor. Last, a check that a pin left at
# -1 where one is needed stops the build.
# Stops at the first run that fails, and identifies it.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
python3 ../embed_web.py --check
SRC="-Imock -I../../firmware/OmniBots_Mini -x c++ harness.cpp"
for v in 2 3; do
  echo "=== ESP32 Arduino core $v.x API, phone joystick on"
  g++ -std=gnu++17 -Wall -DESP32 -DESP_ARDUINO_VERSION_MAJOR=$v $SRC -o build/host_test_v$v
  ./build/host_test_v$v
done
echo "=== phone joystick off (REMOTE_CONTROL 0)"
g++ -std=gnu++17 -Wall -DESP32 -DREMOTE_CONTROL=0 $SRC -o build/host_test_remote_off
./build/host_test_remote_off
echo "=== not an ESP32 board (the phone joystick drops out by itself)"
g++ -std=gnu++17 -Wall $SRC -o build/host_test_not_esp32
./build/host_test_not_esp32
for driver in DRIVER_PH_EN DRIVER_IN1_IN2 DRIVER_PWM_DIR; do
  echo "=== motor driver pins: $driver"
  g++ -std=gnu++17 -Wall -DESP32 -DBOARD_ESP32S3_DEVKIT -DMOTOR_DRIVER=$driver \
    -Imock -I../../firmware/OmniBots_Mini drivers_test.cpp -o build/drivers_$driver
  ./build/drivers_$driver
done
echo "=== a pin left at -1 stops the build"
# my_own's example leaves "direction 2" at -1: with DRIVER_PWM_DIR that must not compile.
if g++ -std=gnu++17 -fsyntax-only -DESP32 -DBOARD_MY_OWN -DMOTOR_DRIVER=DRIVER_PWM_DIR \
    -Imock -I../../firmware/OmniBots_Mini -x c++ ../../firmware/OmniBots_Mini/motors.cpp \
    2> build/pin_check.log; then
  echo "FAIL: DRIVER_PWM_DIR with direction 2 at -1 compiled"
  exit 1
fi
grep -q "direction 2" build/pin_check.log
echo "ok:   DRIVER_PWM_DIR with direction 2 at -1 stops with a message"
echo "ALL PASSED: 0 failure(s)"
