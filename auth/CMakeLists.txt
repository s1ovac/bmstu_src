cmake_minimum_required(VERSION 3.27)
project(auth)

set(CMAKE_CXX_STANDARD 17)

# Подключаем необходимые директории с заголовочными файлами
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/usr/local/include)

# Подключаем Drogon
find_package(Drogon CONFIG REQUIRED)

# Добавляем библиотеку authlib
add_library(authlib
        controllers/AuthController.cpp
        repository/UsersRepo.cpp
        services/AuthService.cpp
        utils/JWT.cpp
)

# Указываем, что authlib зависит от Drogon
target_link_libraries(authlib PRIVATE Drogon::Drogon)

add_executable(auth main.cpp)

# Линкуем библиотеку authlib с основным приложением
target_link_libraries(auth PRIVATE authlib Drogon::Drogon)

# Подключаем библиотеку bcrypt, если требуется
find_library(BCRYPT_LIB bcrypt)
if (BCRYPT_LIB)
    target_link_libraries(auth PRIVATE ${BCRYPT_LIB})
endif()

# Подключаем libpq для работы с PostgreSQL
find_package(PostgreSQL REQUIRED)

target_include_directories(auth PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(auth PRIVATE ${PostgreSQL_LIBRARIES})

# Убедитесь, что libpq правильно подключен
if (NOT PostgreSQL_FOUND)
    message(FATAL_ERROR "PostgreSQL libraries not found")
endif()

find_package(jwt-cpp CONFIG REQUIRED)

target_link_libraries(auth PRIVATE jwt-cpp::jwt-cpp)