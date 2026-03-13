# Comments Spec

## 개요

아티클에 달린 댓글을 추가, 조회, 삭제하는 API.
댓글 추가/삭제는 인증 필요, 댓글 목록 조회는 인증 불필요.
현재 미구현 상태이며, 이 스펙이 구현의 기준이 된다.

## API 엔드포인트

### POST /articles/:slug/comments — 댓글 추가

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
POST /articles/how-to-train-your-dragon/comments
Authorization: Token jwt.token.here
Content-Type: application/json

{
  "comment": {
    "body": "His name was my name too."
  }
}
```

| 필드 | 타입 | 필수 | 설명 |
|------|------|------|------|
| body | string | Y | 댓글 내용 |

#### Response 201 Created

```json
{
  "comment": {
    "id": 1,
    "createdAt": "2016-02-18T03:22:56.637Z",
    "updatedAt": "2016-02-18T03:22:56.637Z",
    "body": "His name was my name too.",
    "author": {
      "username": "jake",
      "bio": "I work at statefarm",
      "image": "https://...",
      "following": false
    }
  }
}
```

#### Response 401 Unauthorized

- 유효하지 않은 토큰

#### Response 404 Not Found

- 존재하지 않는 slug

---

### GET /articles/:slug/comments — 댓글 목록 조회

**인증 불필요** (인증 시 `following` 필드 정확히 반환)

#### Request

```http
GET /articles/how-to-train-your-dragon/comments
```

#### Response 200 OK

```json
{
  "comments": [
    {
      "id": 1,
      "createdAt": "2016-02-18T03:22:56.637Z",
      "updatedAt": "2016-02-18T03:22:56.637Z",
      "body": "His name was my name too.",
      "author": {
        "username": "jake",
        "bio": "I work at statefarm",
        "image": "https://...",
        "following": false
      }
    }
  ]
}
```

#### Response 404 Not Found

- 존재하지 않는 slug

---

### DELETE /articles/:slug/comments/:id — 댓글 삭제

**인증 필요** (댓글 작성자 본인만 삭제 가능)

#### Request

```http
DELETE /articles/how-to-train-your-dragon/comments/1
Authorization: Token jwt.token.here
```

#### Response 200 OK

- 삭제 성공 (빈 응답 본문)

#### Response 401 Unauthorized

- 유효하지 않은 토큰
- 댓글 작성자가 아닌 경우

#### Response 404 Not Found

- 존재하지 않는 댓글 ID 또는 slug

---

## 데이터 모델

```sql
comments (
  id         serial primary key,
  body       text NOT NULL,
  article_id integer references articles,   -- 댓글이 속한 아티클
  user_id    integer references users,      -- 댓글 작성자
  created_at TIMESTAMP NOT NULL,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
)
```

### Comment 응답 객체

```json
{
  "id": 1,
  "body": "댓글 내용",
  "createdAt": "ISO 8601",
  "updatedAt": "ISO 8601",
  "author": {
    "username": "string",
    "bio": "string",
    "image": "string",
    "following": false
  }
}
```

## 비즈니스 규칙

1. **slug → article_id 변환**: slug로 articles 테이블 조회 후 id 획득
2. **작성자 정보 JOIN**: comments + users 조인으로 author 정보 포함
3. **삭제 권한**: JWT에서 추출한 user_id와 comments.user_id 일치 여부 확인
4. **following 필드**: 비인증 요청 시 항상 `false`, 인증 요청 시 follows 테이블 조회

## 구현 상태

| 항목 | 상태 | 비고 |
|------|------|------|
| POST /articles/:slug/comments | 미구현 | Comments 모델은 존재 |
| GET /articles/:slug/comments | 미구현 | - |
| DELETE /articles/:slug/comments/:id | 미구현 | - |

### 구현 시 필요한 작업

1. `api_CommentsController.h/.cc` 생성
2. slug → article_id 조회 헬퍼 함수
3. `comments` Drogon ORM 모델 확인 및 활용
4. author 정보 JOIN 쿼리 작성
