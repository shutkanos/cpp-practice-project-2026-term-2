# 1. Установить MinGW кросс-компилятор
sudo apt update && sudo apt install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64

# 2. Скачать ImGui
mkdir -p vendor && cd vendor
git clone --depth=1 https://github.com/ocornut/imgui.git
cd ..

# 3. Скачать GLFW prebuilt для Windows
#    → https://www.glfw.org/download.html → "Windows pre-compiled binaries"
#    Распаковать в vendor/glfw-windows/
#    Нужны: vendor/glfw-windows/include/ и vendor/glfw-windows/lib-mingw-w64/

# 4. Скачать nlohmann/json (header-only)
mkdir -p vendor/nlohmann/include/nlohmann
wget https://github.com/nlohmann/json/releases/latest/download/json.hpp \
     -O vendor/nlohmann/include/nlohmann/json.hpp

cd vendor

# Скачиваем архив с официального сайта
wget https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip

# Распаковываем (нужен unzip)
sudo apt install unzip -y
unzip glfw-3.4.bin.WIN64.zip

# Переименовываем в ожидаемую папку
mv glfw-3.4.bin.WIN64 glfw-windows

# Проверяем что нужные файлы на месте
ls glfw-windows/include/GLFW/      # должен быть glfw3.h
ls glfw-windows/lib-mingw-w64/     # должен быть libglfw3.a

cd ..

# 5. Сборка
mkdir build-win && cd build-win
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw64.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Результат: build-win/notes.exe
