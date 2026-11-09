#!/bin/sh
# Build the host simulator and render screens to sim/out/. Run from the repo root (WSL or Linux).
set -e
cd "$(dirname "$0")/.."
mkdir -p sim/out
# Address and undefined-behaviour sanitizers: the fuzz test then catches out-of-bounds and similar bugs.
g++ -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -o sim/out/sim sim/sim.cpp components/orbling/pet_engine.cpp -lm
./sim/out/sim sim/out
