---
name: write-unit-test
description: Google Test를 사용한 C++ 단위 테스트 작성. RealWorld API 엔드포인트의 정상/비정상 케이스를 OpenSpec 스펙 기준으로 테스트.
---

RealWorld API 엔드포인트의 단위 테스트를 Google Test / Google Mock으로 작성한다.

## 1단계: 스펙에서 테스트 케이스 정의

```bash
cat openspec/specs/<domain>/spec.md
```

스펙의 다음 항목을 테스트 케이스로 변환한다:

| 스펙 항목 | 테스트 케이스 유형 |
|----------|-----------------|
| Response 200/201 정상 응답 | Happy path |
| Response 400 Bad Request | 유효성 검증 실패 케이스 |
| Response 401 Unauthorized | 인증 누락/만료 케이스 |
| Response 404 Not Found | 존재하지 않는 리소스 케이스 |
| 비즈니스 규칙 | 각 규칙별 경계값 테스트 |

## 2단계: 테스트 파일 구조

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "controllers/api_<Domain>Controller.h"

using namespace api;
using namespace testing;

// 픽스처: 컨트롤러 인스턴스 및 공통 설정
class <Domain>ControllerTest : public Test {
protected:
    void SetUp() override {
        // 테스트 데이터 초기화
    }
    void TearDown() override {
        // 정리
    }
};

// 정상 케이스
TEST_F(<Domain>ControllerTest, <MethodName>_Success) {
    // Arrange: 요청 데이터 준비
    // Act: 컨트롤러 메서드 호출
    // Assert: 응답 상태 코드 및 JSON 바디 검증
}

// 비정상 케이스
TEST_F(<Domain>ControllerTest, <MethodName>_InvalidInput_Returns400) {
    // 필수 필드 누락 시 400 반환 검증
}

TEST_F(<Domain>ControllerTest, <MethodName>_Unauthorized_Returns401) {
    // Authorization 헤더 없는 요청 검증
}
```

## 3단계: 도메인별 주요 테스트 케이스

### Users (GET /user, PUT /user)
- `CurrentUser_WithValidToken_Returns200WithUserJson`
- `CurrentUser_WithoutToken_Returns400`
- `UpdateUser_EmailOnly_UpdatesEmailOnly`
- `UpdateUser_AllFields_UpdatesAllFields`
- `UpdateUser_DuplicateEmail_Returns400`

### Articles (POST /articles, GET /articles)
- `NewArticle_WithRequiredFields_Returns201`
- `NewArticle_MissingTitle_Returns400`
- `GetAllArticles_NoFilter_ReturnsArray`
- `GetAllArticles_WithTagFilter_ReturnsFilteredArray`
- `GetAllArticles_ResponseCode_Is200NotMissing201` ← 알려진 버그 회귀 테스트

### Authentication (POST /users, POST /users/login)
- `NewUser_ValidInput_Returns201WithToken`
- `NewUser_DuplicateEmail_Returns400`
- `Login_ValidCredentials_ReturnsToken`
- `Login_WrongPassword_Returns401`

## 4단계: Google Mock으로 DB 의존성 모킹

DB 호출을 모킹할 때는 Drogon의 DbClient 인터페이스를 활용한다.

```cpp
// Mapper 모킹 예시 (인터페이스 추출이 어려운 경우 통합 테스트 선호)
class MockDbClient : public drogon::orm::DbClient {
public:
    MOCK_METHOD(void, execSqlAsync, (...), (override));
};

// 테스트에서 mock 주입
auto mockClient = std::make_shared<MockDbClient>();
EXPECT_CALL(*mockClient, execSqlAsync(...))
    .WillOnce(InvokeArgument<2>(fakeResult));
```

Drogon 특성상 DB 완전 모킹이 복잡하면 **통합 테스트 방식**(실제 테스트 DB 사용)을 우선 고려한다.

## 5단계: 빌드 및 실행

```bash
# CMakeLists.txt에 테스트 타겟 추가 확인
grep -n "gtest\|gmock\|enable_testing" CMakeLists.txt

# 빌드
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target <test_target>

# 실행
cd build && ctest --verbose
# 또는
./build/<test_binary>
```

## 주의사항

- 각 테스트는 독립적으로 실행 가능해야 한다 (SetUp/TearDown 활용)
- 알려진 버그(`GET /articles` 201 반환, `PUT /user` setter 오타)는 **회귀 테스트**로 먼저 작성하여 버그를 재현한 뒤 수정한다
- 비밀번호 관련 테스트: bcrypt 해싱 확인 (평문 저장 방지)
- 테스트 파일명 규칙: `test_<domain>_controller.cc`
