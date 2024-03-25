echo "Generating build folder for simulator..."
mkdir build_simulator
cd build_simulator
cmake .. -G "Visual Studio 16 2019"
cd ..

pause