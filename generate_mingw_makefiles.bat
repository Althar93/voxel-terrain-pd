echo "Generating release build folder for device..."
mkdir build_device_release
cd build_device_release
cmake .. -G "MinGW Makefiles" --toolchain="%PLAYDATE_SDK_PATH%\C_API\buildsupport\arm.cmake" -DCMAKE_BUILD_TYPE=Release
cd ..

echo "Generating debug build folder for device..."
mkdir build_device_debug
cd build_device_debug
cmake .. -G "MinGW Makefiles" --toolchain="%PLAYDATE_SDK_PATH%\C_API\buildsupport\arm.cmake"
cd ..

pause