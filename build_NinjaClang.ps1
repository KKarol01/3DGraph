cmake -S . -B out -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out --config=Release
cd .\out\src
.\3dcalculator.exe
cd ../../