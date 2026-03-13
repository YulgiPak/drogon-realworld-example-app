# tests/CMakeLists.txt
#
# Google Test 단위 테스트 빌드 설정
# jwt-cpp, libbcrypt 는 루트 CMakeLists.txt 에서 add_subdirectory 로 이미 정의됨.

cmake_minimum_required(VERSION 3.14)

# ── Google Test ──────────────────────────────────────────────────────────────
# 시스템에 설치된 GTest 를 먼저 탐색하고, 없으면 FetchContent 로 다운로드.
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(STATUS "[tests] GTest not found locally — fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # Windows: gtest 가 CRT 공유 설정을 강제하지 않도록
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

# ── 공통 include 경로 ────────────────────────────────────────────────────────
# 루트 디렉토리와 external 서브디렉토리를 포함시켜
# <bcrypt/BCrypt.hpp>, <jwt-cpp/jwt.h> include 경로를 해결.
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ── 헬퍼 함수: 테스트 실행파일 등록 ─────────────────────────────────────────
function(add_realworld_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})

    target_include_directories(${TEST_NAME} PRIVATE
        ${PROJECT_ROOT}
        ${PROJECT_ROOT}/external/jwt-cpp/include
        ${PROJECT_ROOT}/external/libbcrypt/include
    )

    target_compile_features(${TEST_NAME} PRIVATE cxx_std_20)

    target_link_libraries(${TEST_NAME} PRIVATE
        GTest::gtest
        GTest::gtest_main
        jwt-cpp
        bcrypt
    )

    # CTest 에 자동 등록
    gtest_discover_tests(${TEST_NAME})
endfunction()

# ── 테스트 실행파일 등록 ─────────────────────────────────────────────────────

# 1) jwtService 단위 테스트 (jwt-cpp 직접 사용, Drogon 의존성 없음)
add_realworld_test(test_jwtService test_jwtService.cc)

# 2) BCrypt 유틸리티 테스트
add_realworld_test(test_bcrypt test_bcrypt.cc)

# 3) 입력 유효성 검증 테스트 (순수 C++ 로직, 외부 의존성 없음)
add_executable(test_input_validation test_input_validation.cc)
target_compile_features(test_input_validation PRIVATE cxx_std_20)
target_link_libraries(test_input_validation PRIVATE
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(test_input_validation)
