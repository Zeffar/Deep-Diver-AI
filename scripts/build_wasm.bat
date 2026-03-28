@echo off
set ROOT_DIR=%~dp0..
pushd "%ROOT_DIR%"

echo Setting up Emscripten Environment (Just in case)...
call C:\emsdk\emsdk_env.bat
echo Building WebAssembly Module...
call "C:\emsdk\upstream\emscripten\emcc.bat" -lembind -o deep-sea-frontend\public\deep_sea_backend.js cpp\wasm\wasm_bindings.cpp -s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT=web -s EXPORT_NAME="createDeepSeaBackend" -s "EXPORTED_RUNTIME_METHODS=['ExceptionInfo']" -s ALLOW_MEMORY_GROWTH=1 -O3

popd
echo Build Complete.
