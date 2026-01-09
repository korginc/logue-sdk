# how to set up web audio simulator

``
git submodule update --init
``
to get emsdk, and then follow the official instruction: https://emscripten.org/docs/getting_started/downloads.html

``
cd tools/emsdk
./emsdk install latest
./emsdk activate latest
``

# how to compile to wasm and test in browser
Inside your normal project directory, instead of `make install` which creates a hardware specific unit file, run

``
make wasm
``

This will build your audio processor class into wasm, set up a web audio based audio processing graph, and serve the web app via emrun, which is a local server that came with emsdk.
