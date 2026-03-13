---
name: implement-endpoint
description: RealWorld API 엔드포인트를 Drogon C++ 패턴에 맞게 구현. 미구현 엔드포인트 추가 또는 기존 엔드포인트 보완 시 사용.
---

RealWorld API 엔드포인트를 이 프로젝트의 Drogon C++ 패턴에 맞게 구현한다.

## 1단계: 스펙 확인

구현 전 반드시 관련 스펙을 읽는다.

```bash
# 구현 대상 엔드포인트의 스펙 확인
cat openspec/specs/<domain>/spec.md
```

도메인 디렉토리: `users`, `articles`, `authentication`, `comments`, `follows`, `article-favorites`

확인 항목:
- Request/Response 구조 (필드명, 타입, 필수 여부)
- 인증 필요 여부 (`LoginFilter` 적용 대상)
- 상태 코드 (201 Created vs 200 OK 등)
- 비즈니스 규칙 및 구현 상태 표

## 2단계: 기존 패턴 참조

기존 컨트롤러를 읽고 패턴을 파악한다.

```bash
cat controllers/api_ArticlesController.h
cat controllers/api_ArticlesController.cc
cat controllers/api_UsersController.h
cat controllers/api_UsersController.cc
```

### 핵심 패턴

**헤더 파일 — 메서드 등록:**
```cpp
METHOD_LIST_BEGIN
    ADD_METHOD_TO(Controller::method, "/path", HttpMethod, "LoginFilter"); // 인증 필요 시
    ADD_METHOD_TO(Controller::method, "/path", HttpMethod);                // 인증 불필요 시
METHOD_LIST_END
```

**구현 파일 — 비동기 콜백 패턴:**
```cpp
auto Controller::method(const HttpRequestPtr &req,
                        function<void(const HttpResponsePtr &)> &&callback) -> void {
    auto callbackPtr = make_shared<function<void(const HttpResponsePtr &)>>(move(callback));
    // DB 작업...
    mapper.findOne(criteria,
        [callbackPtr](const Model &obj) {
            auto json = Json::Value();
            json["key"] = obj.toJson();
            auto resp = HttpResponse::newHttpJsonResponse(json);
            resp->setStatusCode(HttpStatusCode::k200OK);
            (*callbackPtr)(resp);
        },
        [callbackPtr](const DrogonDbException &e) {
            LOG_ERROR << e.base().what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(HttpStatusCode::k400BadRequest);
            (*callbackPtr)(resp);
        }
    );
}
```

**JWT 인증이 필요한 엔드포인트:**
```cpp
#include <utils/jwtService.h>

jwtService::getCurrentUserFromRequest(req, [=](optional<Users> user) {
    if (!user.has_value()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(HttpStatusCode::k401Unauthorized);
        (*callbackPtr)(resp);
        return;
    }
    // user.value() 사용
});
```

**Raw SQL 쿼리 패턴 (JOIN 필요 시):**
```cpp
auto dbClient = app().getFastDbClient();
*dbClient << "SELECT ... FROM ... WHERE $1 = ..."
          << param
          >> [callbackPtr](const Result &r) { /* 성공 처리 */ }
          >> [callbackPtr](const DrogonDbException &e) { /* 오류 처리 */ };
```

## 3단계: LoginFilter 적용 여부 결정

| 조건 | 처리 |
|------|------|
| 스펙에 "인증 필요" 명시 | `ADD_METHOD_TO`에 `"LoginFilter"` 추가 |
| 스펙에 "인증 불필요" 명시 | `"LoginFilter"` 생략 |
| 스펙에 "선택적 인증" 명시 | `"LoginFilter"` 생략 후 핸들러에서 수동 파싱 |

## 4단계: 상태 코드 확인

스펙의 Response 절에 명시된 상태 코드를 정확히 사용한다.

| 상황 | 상태 코드 |
|------|----------|
| 리소스 생성 성공 | `k201Created` |
| 조회/수정 성공 | `k200OK` |
| 유효성 검증 실패 | `k400BadRequest` |
| 인증 실패 | `k401Unauthorized` |
| 권한 없음 | `k403Forbidden` |
| 리소스 없음 | `k404NotFound` |
| DB/서버 오류 | `k500InternalServerError` |

## 5단계: CMakeLists.txt 확인

새 컨트롤러 파일을 추가한 경우 `CMakeLists.txt`에 소스 파일이 포함되어 있는지 확인한다.

```bash
grep -n "controllers" CMakeLists.txt
```
