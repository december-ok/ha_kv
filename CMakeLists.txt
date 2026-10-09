cmake_minimum_required(VERSION 3.30)
project(AsioApp CXX)

# C++23 표준 설정
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# 1. Boost 최신 Config 모드로 탐색 (CMP0167 경고 원천 차단)
find_package(Boost REQUIRED CONFIG)

# 2. 실행 파일 생성
add_executable(my_asio_app src/main.cpp)

# 3. Target 기반 의존성 연결
target_link_libraries(my_asio_app PRIVATE Boost::headers)

# 4. 멀티스레드 지원
find_package(Threads REQUIRED)
target_link_libraries(my_asio_app PRIVATE Threads::Threads)
