cmake_minimum_required(VERSION 3.15)
project(fileservice)

# Устанавливаем стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Добавляем определение для исключения SQLite3
add_definitions(-DDROGON_NO_SQLITE3)

# Находим необходимые пакеты
find_package(Drogon REQUIRED)
find_package(PostgreSQL REQUIRED)

# Добавляем исполняемый файл и исходные файлы
add_executable(fileservice
        main.cc
        pkg/db.cc
        controllers/FileController.cc
        controllers/AdminController.cpp
        filters/JwtAuthFilter.cc
        filters/PermissionFilter.cpp
        services/FileService.cc
        services/AdminService.cpp
        pkg/jwt_utils.cpp
        pkg/permission_utils.cpp
        # Добавьте другие файлы при необходимости
)

# Указываем директории заголовочных файлов
target_include_directories(fileservice PRIVATE
        ${DROGON_INCLUDE_DIRS}
        ${PostgreSQL_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        pkg
)

# Связываем необходимые библиотеки
target_link_libraries(fileservice PRIVATE
        ${DROGON_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
)

# Проверяем использование стандарта C++17
if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "C++17 or higher is required")
else()
    message(STATUS "Using C++${CMAKE_CXX_STANDARD}")
endif()

# Проверяем поддержку std::filesystem
include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
    #include <filesystem>
    namespace fs = std::filesystem;
    int main() {
        fs::path p{\".\"};
        return 0;
    }
" HAS_FILESYSTEM)

if(NOT HAS_FILESYSTEM)
    message(FATAL_ERROR "Your compiler does not support std::filesystem")
endif()

# Для поддержки std::filesystem на Linux
if (UNIX AND NOT APPLE)
    target_link_libraries(fileservice PRIVATE stdc++fs)
endif()

# Если у вас есть представления (views) в Drogon
drogon_create_views(fileservice ${CMAKE_CURRENT_SOURCE_DIR}/views ${CMAKE_CURRENT_BINARY_DIR})

# Если необходимо, используйте aux_source_directory для дополнительных исходников
# aux_source_directory(controllers CTL_SRC)
# aux_source_directory(filters FILTER_SRC)
# aux_source_directory(plugins PLUGIN_SRC)
# aux_source_directory(models MODEL_SRC)

# Добавляем исходные файлы из директорий
# target_sources(fileservice PRIVATE
#     ${CTL_SRC}
#     ${FILTER_SRC}
#     ${PLUGIN_SRC}
#     ${MODEL_SRC}
# )

# Удалите или раскомментируйте следующие строки, если у вас есть тесты
# add_subdirectory(test)
