#!/usr/bin/env bash
# =====================================================================================
#  check_all.sh - build everything, run every check, and report the results
# =====================================================================================
#
#  Run it from any folder, before sharing a change:
#
#      tools/check_all.sh
#
#  What it does, in order:
#    0. checks web_assets.h matches web/ (first: every build below rewrites it from web/,
#       and the Arduino IDE uses the file as it is in the folder)
#    1. builds the full program for every board in platformio.ini, phone joystick on
#    2. builds the ESP32 boards again with the phone joystick off (-DREMOTE_CONTROL=0),
#       checks no WiFi library went in, and builds the Romeo once more with the switch
#       set the Arduino IDE way: REMOTE_CONTROL 0 typed into a copy of config.h
#    3. builds the five lessons for the Romeo, the UNO R4 and the classic Uno (R3)
#    4. runs the host test: the program on this computer, with simulated motors, and the
#       motor driver pin checks
#    5. checks that every picture in the docs is shown or hidden as it should be
#  Then it prints a table, and finishes with "ALL PASSED" (exit code 0) or "FAILED"
#  (exit code 1). A compiler warning in the project's own files also counts as a failure.
#
#  It needs PlatformIO with the boards already installed (it uses ~/.platformio/penv
#  if `pio` is not on the PATH). It never uploads anything and never opens a port.
#  The full log of every step is in .pio/check_all/, for use if something fails.
#
# =====================================================================================

set -u
cd "$(dirname "$0")/.."   # the project folder

PIO="$(command -v pio || echo "$HOME/.platformio/penv/bin/pio")"
LOGS=".pio/check_all"
mkdir -p "$LOGS"

BOARDS="romeo esp32_devkit esp32s3_devkit arduino_mega uno_r4_minima uno_r4_wifi stm32_nucleo my_own"
ESP32_BOARDS="romeo esp32_devkit esp32s3_devkit my_own"
LESSON_BOARDS="romeo uno_r4_minima"

ROWS=()      # one line per check, for the table at the end
FAILED=0

# One row of the table: what, phone joystick, flash, RAM, result
add_row() {
  ROWS+=("$(printf '%-46s %-6s %10s %8s  %s' "$1" "$2" "$3" "$4" "$5")")
  if [ "$5" != "PASS" ]; then FAILED=1; fi
}

# Bytes used, from PlatformIO's "RAM: ... (used 45900 bytes ...)" lines.
used() {
  grep -E "^$1:" "$2" | sed -E 's/.*used ([0-9]+) bytes.*/\1/' | tail -1
}

# Compiler warnings in the project's own files: every warning that is not about a file in
# the board packages PlatformIO installed (~/.platformio/packages).
our_warnings() {
  grep -E "warning:" "$1" | grep -c -v -E "\.platformio/packages|framework-" || true
}

# Builds once and adds a row. $1 = label, $2 = phone joystick, $3 = log, then the command.
# With the phone joystick off, a library in the build (PlatformIO lists them under
# "Dependency Graph") means WiFi code got in, so that fails too.
build() {
  local label="$1" remote="$2" log="$3"
  shift 3
  "$@" > "$log" 2>&1
  local status=$?
  local flash ram warnings result
  flash="$(used Flash "$log")"
  ram="$(used RAM "$log")"
  warnings="$(our_warnings "$log")"
  if grep -q -E "Downloading|Installing" "$log"; then
    result="FAIL (it tried to download something, see $log)"
  elif [ $status -ne 0 ]; then
    result="FAIL (see $log)"
  elif [ "$warnings" -gt 0 ]; then
    result="FAIL ($warnings warning(s) in our files, see $log)"
  elif [ "$remote" = "off" ] && grep -q "Dependency Graph" "$log"; then
    result="FAIL (libraries in a phone-joystick-off build, see $log)"
  else
    result="PASS"
  fi
  add_row "$label" "$remote" "${flash:--}" "${ram:--}" "$result"
  echo "  $label ($remote): $result"
}

echo "== 0. The packed driving page"
python3 tools/embed_web.py --check > "$LOGS/embed_web.log" 2>&1
if [ $? -eq 0 ]; then result="PASS"; else result="FAIL (see $LOGS/embed_web.log)"; fi
add_row "web_assets.h matches web/" "-" "-" "-" "$result"
echo "  web_assets.h: $result"

echo "== 1. The full program, phone joystick on"
for env in $BOARDS; do
  build "$env" "on" "$LOGS/$env.log" "$PIO" run -e "$env"
done

echo "== 2. The ESP32 boards, phone joystick off"
for env in $ESP32_BOARDS; do
  # A separate build folder, so that the two kinds of build do not rebuild each other.
  build "$env" "off" "$LOGS/${env}_remote_off.log" \
    env PLATFORMIO_BUILD_FLAGS=-DREMOTE_CONTROL=0 PLATFORMIO_BUILD_DIR=.pio/build_remote_off \
    "$PIO" run -e "$env"
done
# The Arduino IDE way: REMOTE_CONTROL 0 typed into config.h itself, in a copy of the sketch.
COPY="$LOGS/config_off/OmniBots_Mini"
rm -rf "$LOGS/config_off"
mkdir -p "$COPY"
cp firmware/OmniBots_Mini/* "$COPY/"
sed -i 's/^#define REMOTE_CONTROL 1$/#define REMOTE_CONTROL 0/' "$COPY/config.h"
if grep -q "^#define REMOTE_CONTROL 0$" "$COPY/config.h"; then
  build "romeo, REMOTE_CONTROL 0 in config.h" "off" "$LOGS/romeo_config_off.log" \
    env PLATFORMIO_SRC_DIR="$COPY" PLATFORMIO_BUILD_DIR=.pio/build_config_off \
    "$PIO" run -e romeo
else
  add_row "romeo, REMOTE_CONTROL 0 in config.h" "off" "-" "-" "FAIL (no REMOTE_CONTROL 1 line in config.h)"
fi

echo "== 3. The lessons"
for lesson in firmware/lessons/*/; do
  name="$(basename "$lesson")"
  for env in $LESSON_BOARDS; do
    build "lesson $name, $env" "-" "$LOGS/lesson_${name}_$env.log" \
      env PLATFORMIO_SRC_DIR="$lesson" PLATFORMIO_BUILD_DIR=".pio/lessons/$env/$name" \
      "$PIO" run -e "$env"
  done
  # The classic Uno has no section in platformio.ini (the full program does not fit on it),
  # so the lessons are built for it in a throwaway project.
  build "lesson $name, uno (R3)" "-" "$LOGS/lesson_${name}_uno.log" \
    "$PIO" ci --board uno "$lesson"
done

echo "== 4. The host test"
tools/host_test/run.sh > "$LOGS/host_test.log" 2>&1
status=$?
passes="$(grep -c "ALL PASSED" "$LOGS/host_test.log")"
runs=8   # the four program runs, three motor driver runs and the pin check in run.sh
warnings="$(our_warnings "$LOGS/host_test.log")"
if [ $status -eq 0 ] && [ "$passes" -eq $runs ] && [ "$warnings" -eq 0 ]; then
  result="PASS"
else
  result="FAIL (see $LOGS/host_test.log)"
fi
add_row "host test ($passes of $runs runs passed)" "on+off" "-" "-" "$result"
echo "  host test: $result"

echo "== 5. Pictures"
python3 tools/show_pictures.py --check > "$LOGS/show_pictures.log" 2>&1
if [ $? -eq 0 ]; then result="PASS"; else result="FAIL (see $LOGS/show_pictures.log)"; fi
add_row "pictures shown or hidden right" "-" "-" "-" "$result"

echo
printf '%-46s %-6s %10s %8s  %s\n' "Check" "Phone" "Flash (B)" "RAM (B)" "Result"
printf '%-46s %-6s %10s %8s  %s\n' "-----" "-----" "---------" "-------" "------"
for row in "${ROWS[@]}"; do echo "$row"; done
echo
if [ $FAILED -eq 0 ]; then
  echo "ALL PASSED"
  exit 0
fi
echo "FAILED: see the rows above, and their logs in $LOGS/"
exit 1
