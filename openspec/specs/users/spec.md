# Users Spec

## 개요

인증된 사용자의 프로필 정보를 조회하고 수정하는 API. 두 엔드포인트 모두 `LoginFilter`를 통해
JWT 인증을 요구한다. 요청 헤더의 JWT 토큰으로 현재 사용자를 식별한다.

## API 엔드포인트

### GET /user — 현재 사용자 조회

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
GET /user
Authorization: Token jwt.token.here
```

#### Response 200 OK

```json
{
  "user": {
    "email": "jake@jake.jake",
    "token": "jwt.token.here",
    "username": "jacob",
    "bio": "I work at statefarm",
    "image": "https://..."
  }
}
```

#### Response 401 Unauthorized

- `Authorization` 헤더 없음
- 토큰이 유효하지 않거나 만료됨

#### Response 400 Bad Request

- 토큰은 유효하나 대응하는 사용자가 DB에 없는 경우

---

### PUT /user — 사용자 정보 수정

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
PUT /user
Authorization: Token jwt.token.here
Content-Type: application/json

{
  "user": {
    "email": "jake@jake.jake",
    "bio": "I like to skateboard",
    "image": "https://i.stack.imgur.com/xHWG8.jpg"
  }
}
```

| 필드 | 타입 | 필수 | 설명 |
|------|------|------|------|
| email | string | N | 변경할 이메일 |
| username | string | N | 변경할 사용자명 |
| password | string | N | 변경할 비밀번호 |
| bio | string | N | 변경할 소개글 |
| image | string | N | 변경할 프로필 이미지 URL |

모든 필드는 선택 사항. 전송된 필드만 업데이트한다.

#### Response 200 OK

```json
{
  "user": {
    "email": "jake@jake.jake",
    "token": "jwt.token.here",
    "username": "jacob",
    "bio": "I like to skateboard",
    "image": "https://i.stack.imgur.com/xHWG8.jpg"
  }
}
```

#### Response 401 Unauthorized

- 유효하지 않은 토큰

#### Response 400 Bad Request

- DB 업데이트 실패 (중복 email/username 등)

---

## 데이터 모델

```sql
users (
  id       serial primary key,
  username text UNIQUE,
  email    text UNIQUE,
  password text,   -- bcrypt 해시
  bio      text,
  image    text
)
```

### UserWithToken 응답 구조체

`api_UsersController.h`의 내부 구조체. 응답 JSON 직렬화에 사용.

```cpp
struct UserWithToken {
    string email;
    string username;
    string bio;
    string image;
    string token;   // 현재 요청의 JWT 토큰을 재발급
};
```

## 비즈니스 규칙

1. **사용자 식별**: `jwtService::getCurrentUserFromRequest(req)` — 토큰에서 user_id 추출 후 DB 조회
2. **부분 업데이트**: 전송된 필드만 수정. `nullptr` 체크로 미전송 필드를 스킵
3. **비밀번호 변경**: 신규 비밀번호도 bcrypt 해시 처리 필요 (현재 미구현)
4. **토큰 재발급**: GET/PUT 모두 응답 시 최신 토큰을 재발급하여 반환

## 구현 상태

| 항목 | 상태 | 비고 |
|------|------|------|
| GET /user | 완료 | `api_UsersController::currentUser()` |
| PUT /user | 완료 (버그) | `api_UsersController::update()` |
| 부분 업데이트 로직 | 버그 있음 | 모든 필드 수정이 `setEmail()`로 잘못 연결됨 |
| 비밀번호 변경 시 bcrypt | 미구현 | 현재 평문 저장 위험 |
| 중복 email/username 검증 | 미구현 | DB UNIQUE 제약 위반 시 400 반환만 함 |

### 버그 상세: PUT /user update()

`update()` 메서드에서 `username`, `password`, `bio`, `image` 수정 시 모두 `user->setEmail()`을 잘못 호출:

```cpp
// 버그: setEmail() 대신 setUsername(), setPassword(), setBio(), setImage() 여야 함
if (pNewUser.getUsername() != nullptr) {
    user->setEmail(pNewUser.getValueOfUsername());  // 잘못된 setter
}
```
