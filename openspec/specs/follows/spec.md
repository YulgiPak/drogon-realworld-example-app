# Follows Spec

## 개요

다른 사용자를 팔로우/언팔로우하고 프로필을 조회하는 API.
팔로우/언팔로우는 인증 필요, 프로필 조회는 인증 불필요.
현재 미구현 상태이며, 이 스펙이 구현의 기준이 된다.

## API 엔드포인트

### GET /profiles/:username — 프로필 조회

**인증 불필요** (인증 시 `following` 정확히 반환)

#### Request

```http
GET /profiles/jake
Authorization: Token jwt.token.here   (선택)
```

#### Response 200 OK

```json
{
  "profile": {
    "username": "jake",
    "bio": "I work at statefarm",
    "image": "https://...",
    "following": false
  }
}
```

#### Response 404 Not Found

- 존재하지 않는 username

---

### POST /profiles/:username/follow — 팔로우

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
POST /profiles/jake/follow
Authorization: Token jwt.token.here
```

Request body 없음.

#### Response 200 OK

```json
{
  "profile": {
    "username": "jake",
    "bio": "I work at statefarm",
    "image": "https://...",
    "following": true
  }
}
```

#### Response 401 Unauthorized

- 유효하지 않은 토큰

#### Response 404 Not Found

- 존재하지 않는 username

#### Response 422 Unprocessable Entity

- 이미 팔로우 중인 경우 (선택적)

---

### DELETE /profiles/:username/follow — 언팔로우

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
DELETE /profiles/jake/follow
Authorization: Token jwt.token.here
```

#### Response 200 OK

```json
{
  "profile": {
    "username": "jake",
    "bio": "I work at statefarm",
    "image": "https://...",
    "following": false
  }
}
```

#### Response 401 Unauthorized

- 유효하지 않은 토큰

#### Response 404 Not Found

- 존재하지 않는 username 또는 팔로우 관계 없음

---

## 데이터 모델

```sql
follows (
  user_id   integer not null references users,   -- 팔로우 하는 사람
  follow_id integer not null references users    -- 팔로우 받는 사람
)
```

### Profile 응답 객체

```json
{
  "username": "string",
  "bio": "string | null",
  "image": "string | null",
  "following": true | false
}
```

### following 필드 계산

```sql
SELECT EXISTS (
  SELECT 1 FROM follows
  WHERE user_id = :currentUserId AND follow_id = :targetUserId
) AS following
```

## 비즈니스 규칙

1. **자기 자신 팔로우 방지**: `user_id != follow_id` 조건 확인 권장
2. **중복 팔로우**: follows 테이블에 UNIQUE 제약이 없으므로 삽입 전 존재 여부 확인 필요
3. **following 필드**: 비인증 요청 시 항상 `false`; 인증 시 follows 테이블에서 조회
4. **username → user_id 변환**: 경로 파라미터 username으로 users 테이블 조회

## 구현 상태

| 항목 | 상태 | 비고 |
|------|------|------|
| GET /profiles/:username | 미구현 | Follows 모델은 존재 |
| POST /profiles/:username/follow | 미구현 | - |
| DELETE /profiles/:username/follow | 미구현 | - |

### 구현 시 필요한 작업

1. `api_ProfilesController.h/.cc` 생성
2. username → user_id 조회 헬퍼
3. `follows` Drogon ORM 모델 확인
4. following 상태 확인 쿼리
5. follows 테이블 UNIQUE 제약 추가 고려 (현재 스키마에 없음)

### 스키마 개선 제안

```sql
-- 중복 팔로우 방지를 위한 복합 PK 추가
ALTER TABLE follows ADD PRIMARY KEY (user_id, follow_id);
```
