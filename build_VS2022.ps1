cmake -S . -B out -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
cmake --build out --config=Release
Copy-Item .\out\src\Release\3dcalculator.exe .\out\src -Force
cd .\out\src
.\3dcalculator.exe
cd ../../
