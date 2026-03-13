---
name: fix-realworld-bug
description: drogon-realworld-example-app의 알려진 버그 수정. PUT /user setter 오타, GET /articles 상태코드 버그 등 스펙과 구현 간 불일치 수정 시 사용.
---

이 프로젝트의 알려진 버그를 수정한다. 수정 전 반드시 해당 파일을 읽고 현재 상태를 확인한다.

## 알려진 버그 목록

### Bug 1: PUT /user — setter 오타 (심각도: 높음)

**파일:** `controllers/api_UsersController.cc`
**함수:** `UsersController::update()`
**증상:** `username`, `password`, `bio`, `image` 필드를 수정해도 실제로는 `email` 필드만 덮어씌워진다.

**원인:** 모든 필드 업데이트에서 올바른 setter 대신 `setEmail()`을 잘못 호출.

**현재 코드 (버그):**
```cpp
if (pNewUser.getUsername() != nullptr) {
    user->setEmail(pNewUser.getValueOfUsername());  // 잘못됨
}
if (pNewUser.getPassword() != nullptr) {
    user->setEmail(pNewUser.getValueOfPassword());  // 잘못됨
}
if (pNewUser.getBio() != nullptr) {
    user->setEmail(pNewUser.getValueOfBio());       // 잘못됨
}
if (pNewUser.getImage() != nullptr) {
    user->setEmail(pNewUser.getValueOfImage());     // 잘못됨
}
```

**수정 방법:** 각 필드에 맞는 setter로 교체:
```cpp
if (pNewUser.getUsername() != nullptr) {
    user->setUsername(pNewUser.getValueOfUsername());
}
if (pNewUser.getPassword() != nullptr) {
    user->setPassword(pNewUser.getValueOfPassword());
    // TODO: bcrypt 해싱 추가 필요 (Bug 3 참조)
}
if (pNewUser.getBio() != nullptr) {
    user->setBio(pNewUser.getValueOfBio());
}
if (pNewUser.getImage() != nullptr) {
    user->setImage(pNewUser.getValueOfImage());
}
```

---

### Bug 2: GET /articles — 응답 상태코드 201 오류 (심각도: 중간)

**파일:** `controllers/api_ArticlesController.cc`
**함수:** `ArticlesController::getAllArticles()`
**증상:** 목록 조회(`GET /articles`)가 `201 Created`를 반환. RealWorld 스펙은 `200 OK`를 요구.

**영향 범위:** 4개의 코드 경로 모두 동일하게 잘못됨:
- tag 필터 분기 (line ~54)
- favorited 필터 분기 (line ~92)
- author 필터 분기 (line ~128)
- 전체 조회 분기 (line ~147)

**수정 방법:** `getAllArticles()` 내의 모든 성공 응답에서 상태코드 변경:
```cpp
// 변경 전
resp->setStatusCode(HttpStatusCode::k201Created);
// 변경 후
resp->setStatusCode(HttpStatusCode::k200OK);
```

수정 후 `newArticle()`의 201은 올바르므로 건드리지 않는다.

---

### Bug 3: PUT /user — 비밀번호 평문 저장 (심각도: 높음, 보안)

**파일:** `controllers/api_UsersController.cc`
**함수:** `UsersController::update()`
**증상:** 비밀번호 변경 시 bcrypt 해싱 없이 평문 저장.

**참고:** `newUser()`에서는 bcrypt 처리가 올바르게 구현되어 있음:
```cpp
// newUser()의 올바른 구현 (참조용)
#include <external/libbcrypt/include/bcrypt/BCrypt.hpp>
auto encryptedPass = BCrypt::generateHash(newUser.getValueOfPassword());
newUser.setPassword(encryptedPass);
```

**수정 방법:** `update()` 내 비밀번호 처리에 동일한 패턴 적용:
```cpp
if (pNewUser.getPassword() != nullptr) {
    auto encryptedPass = BCrypt::generateHash(pNewUser.getValueOfPassword());
    user->setPassword(encryptedPass);
}
```

`BCrypt.hpp` include는 파일 상단에 이미 있음 (`#include <external/libbcrypt/include/bcrypt/BCrypt.hpp>`).

---

## 수정 절차

1. **수정 전 파일 읽기:**
   ```bash
   cat controllers/api_UsersController.cc
   cat controllers/api_ArticlesController.cc
   ```

2. **버그 재현 확인** (가능한 경우): 테스트 또는 curl 요청으로 버그 동작 확인

3. **최소 변경 원칙**: 버그와 직접 관련된 줄만 수정. 리팩토링은 별도 작업으로 분리.

4. **수정 후 검증:**
   ```bash
   # 빌드 확인
   cmake --build build
   # 해당 엔드포인트 수동 테스트
   curl -X PUT http://localhost:3000/user \
     -H "Authorization: Token <jwt>" \
     -H "Content-Type: application/json" \
     -d '{"user": {"username": "newname"}}'
   ```

## 미구현 항목 (버그 아님, 별도 구현 필요)

- `GET /articles/:slug` — 단일 아티클 조회
- `PUT /articles/:slug` — 아티클 수정
- `DELETE /articles/:slug` — 아티클 삭제
- `GET /articles/feed` — 팔로우 피드
- `GET /tags` — 태그 목록
- `articlesCount` 필드 — GET /articles 응답에 `{"articles": [...], "articlesCount": N}` 구조 추가

이 항목들은 `implement-endpoint` 스킬로 처리한다.
