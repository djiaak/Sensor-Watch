#!/bin/bash
rm -rf build
emmake make
python3 -m http.server -d build/
