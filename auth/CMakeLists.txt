cmake_minimum_required(VERSION 3.27)
project(auth)

set(CMAKE_CXX_STANDARD 17)

# Подключаем необходимые директории с заголовочными файлами
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/usr/local/include)

# Находим необходимые пакеты
find_package(Drogon REQUIRED)
find_package(PostgreSQL REQUIRED)

add_executable(auth
        main.cpp
        controllers/AuthController.cpp
        controllers/RoleController.cpp
        controllers/GroupController.cpp
        controllers/AdminController.cpp
        controllers/PermissionController.cpp
        repository/DB.cpp
        services/AuthService.cpp
        services/AccessControlService.cpp
        services/RoleService.cpp
        services/GroupService.cpp
        filters/JwtAuthFilter.cpp
        utils/JWT.cpp
)

# Указываем директории заголовочных файлов
target_include_directories(auth PRIVATE
        ${DROGON_INCLUDE_DIRS}
        ${PostgreSQL_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Связываем необходимые библиотеки
target_link_libraries(auth PRIVATE
        ${DROGON_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
)

# Проверяем использование стандарта C++17
if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "C++17 or higher is required")
else()
    message(STATUS "Using C++${CMAKE_CXX_STANDARD}")
endif()

# Подключаем библиотеку bcrypt, если требуется
find_library(BCRYPT_LIB bcrypt)
if (BCRYPT_LIB)
    target_link_libraries(auth PRIVATE ${BCRYPT_LIB})
endif()

target_include_directories(auth PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(auth PRIVATE ${PostgreSQL_LIBRARIES})

# Убедитесь, что libpq правильно подключен
if (NOT PostgreSQL_FOUND)
    message(FATAL_ERROR "PostgreSQL libraries not found")
endif()

find_package(jwt-cpp CONFIG REQUIRED)

target_link_libraries(auth PRIVATE jwt-cpp::jwt-cpp)