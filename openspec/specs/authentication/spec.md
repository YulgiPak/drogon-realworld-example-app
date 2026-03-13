# Authentication Spec

## 개요

회원가입 및 로그인을 통해 JWT 토큰을 발급한다. 발급된 토큰은 이후 인증이 필요한 모든 API 요청의
`Authorization` 헤더에 포함한다. 토큰 검증은 `LoginFilter`가 담당하며, 검증 실패 시 `401 Unauthorized`를 반환한다.

## API 엔드포인트

### POST /users — 회원가입

**인증 불필요**

#### Request

```http
POST /users
Content-Type: application/json

{
  "user": {
    "username": "jacob",
    "email": "jake@jake.jake",
    "password": "jakejake"
  }
}
```

| 필드 | 타입 | 필수 | 설명 |
|------|------|------|------|
| username | string | Y | 고유 사용자명 |
| email | string | Y | 고유 이메일 |
| password | string | Y | 평문 비밀번호 (서버에서 bcrypt 해시) |

#### Response 201 Created

```json
{
  "user": {
    "email": "jake@jake.jake",
    "token": "jwt.token.here",
    "username": "jacob",
    "bio": "",
    "image": "https://default-image-url"
  }
}
```

#### Response 400 Bad Request

- 필수 필드 누락 (email, password, username 중 하나라도 없는 경우)
- 이미 존재하는 username 또는 email

---

### POST /users/login — 로그인

**인증 불필요**

#### Request

```http
POST /users/login
Content-Type: application/json

{
  "user": {
    "email": "jake@jake.jake",
    "password": "jakejake"
  }
}
```

| 필드 | 타입 | 필수 | 설명 |
|------|------|------|------|
| email | string | Y | 등록된 이메일 |
| password | string | Y | 비밀번호 |

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

- 비밀번호 불일치

#### Response 400 Bad Request

- 존재하지 않는 이메일

---

## 인증 토큰 사용법

발급받은 토큰은 이후 요청 시 아래와 같이 헤더에 포함한다.

```http
Authorization: Token jwt.token.here
```

- `Bearer` 방식이 아닌 `Token` 키워드 사용 (RealWorld 스펙 규칙)
- `LoginFilter`가 헤더에서 토큰을 추출하여 `jwt-cpp`로 서명 검증
- 검증 실패(토큰 없음, 만료, 위변조) 시 `401 Unauthorized` 반환

## 데이터 모델

```sql
users (
  id       serial primary key,
  username text UNIQUE NOT NULL,
  email    text UNIQUE NOT NULL,
  password text NOT NULL,   -- bcrypt 해시값
  bio      text,
  image    text              -- 기본값: config.json의 image-default
)
```

## 비즈니스 규칙

1. **비밀번호 저장**: 평문 저장 금지. `BCrypt::generateHash()`로 해시 후 저장
2. **비밀번호 검증**: `BCrypt::validatePassword(입력값, 해시값)`으로 검증
3. **중복 검사**: username과 email 모두 UNIQUE 제약 — 가입 전 DB 조회로 중복 확인
4. **기본 이미지**: 신규 가입 시 `config.json`의 `image-default` 값으로 설정
5. **토큰 발급**: 회원가입/로그인 성공 시 `jwtService::generateFromUser(user)` 호출

## 구현 상태

| 항목 | 상태 | 비고 |
|------|------|------|
| POST /users (회원가입) | 완료 | `api_UsersController::newUser()` |
| POST /users/login (로그인) | 완료 | `api_UsersController::login()` |
| JWT 생성 | 완료 | `jwtService::generateFromUser()` |
| JWT 검증 필터 | 완료 | `LoginFilter::doFilter()` |
| 토큰 만료 처리 | 미확인 | `jwtService` 내부 `duration` 설정 존재 |
| 에러 응답 body 상세화 | 미구현 | 현재 빈 body로 상태코드만 반환 |
