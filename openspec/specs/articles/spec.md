# Articles Spec

## 개요

아티클(게시글) CRUD API. 아티클 생성은 인증 필요, 목록 조회는 인증 불필요.
tag, favorited, author 필터와 offset/limit 페이지네이션을 지원한다.
현재 생성(POST)과 목록 조회(GET)만 구현되어 있다.

## API 엔드포인트

### POST /articles — 아티클 생성

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
POST /articles
Authorization: Token jwt.token.here
Content-Type: application/json

{
  "article": {
    "title": "How to train your dragon",
    "description": "Ever wonder how?",
    "body": "You have to believe",
    "tagList": ["reactjs", "angularjs", "dragons"]
  }
}
```

| 필드 | 타입 | 필수 | 설명 |
|------|------|------|------|
| title | string | Y | 아티클 제목 |
| description | string | Y | 짧은 설명 |
| body | string | Y | 본문 내용 |
| tagList | string[] | N | 태그 목록 |

#### Response 201 Created

```json
{
  "article": {
    "slug": "how-to-train-your-dragon",
    "title": "How to train your dragon",
    "description": "Ever wonder how?",
    "body": "You have to believe",
    "tagList": ["reactjs", "angularjs", "dragons"],
    "createdAt": "2016-02-18T03:22:56.637Z",
    "updatedAt": "2016-02-18T03:48:35.824Z",
    "author": { "username": "...", "bio": "...", "image": "...", "following": false },
    "favorited": false,
    "favoritesCount": 0
  }
}
```

#### Response 400 Bad Request

- DB 삽입 실패 (slug 중복 등)

---

### GET /articles — 아티클 목록 조회

**인증 불필요** (선택적 인증 가능)

#### Request

```http
GET /articles?tag=dragons&author=jacob&favorited=jane&limit=20&offset=0
```

| 쿼리 파라미터 | 타입 | 설명 |
|--------------|------|------|
| tag | string | 특정 태그가 포함된 아티클 필터 |
| author | string | 특정 작성자(username)의 아티클 필터 |
| favorited | string | 특정 사용자(username)가 즐겨찾기한 아티클 필터 |
| offset | integer | 건너뛸 레코드 수 (기본값: 0) |
| limit | integer | 반환할 최대 레코드 수 (기본값: 0 = 제한 없음) |

- tag, author, favorited는 상호 배타적으로 동작 (우선순위: tag > favorited > author)
- 모두 없으면 전체 목록 반환

#### Response 200 OK (현재 구현은 201 반환 — 버그)

```json
[
  {
    "slug": "how-to-train-your-dragon",
    "title": "How to train your dragon",
    "description": "Ever wonder how?",
    "body": "You have to believe",
    "tagList": ["reactjs", "dragons"],
    "createdAt": "2016-02-18T03:22:56.637Z",
    "updatedAt": "2016-02-18T03:48:35.824Z"
  }
]
```

---

### GET /articles/:slug — 아티클 상세 조회 (미구현)

**인증 불필요**

```http
GET /articles/how-to-train-your-dragon
```

Response 200: 단일 article 객체

---

### PUT /articles/:slug — 아티클 수정 (미구현)

**인증 필요**

```http
PUT /articles/how-to-train-your-dragon
```

---

### DELETE /articles/:slug — 아티클 삭제 (미구현)

**인증 필요**

```http
DELETE /articles/how-to-train-your-dragon
```

---

### GET /articles/feed — 팔로우 피드 (미구현)

**인증 필요**

팔로우하는 사용자들의 아티클만 최신순으로 반환.

```http
GET /articles/feed?limit=20&offset=0
```

---

## 데이터 모델

```sql
articles (
  id          serial primary key,
  user_id     integer references users,   -- 작성자
  slug        text UNIQUE,                -- URL-friendly 제목
  title       text,
  description text,
  body        text,
  tagList     text[],                     -- PostgreSQL 배열 타입
  created_at  TIMESTAMP NOT NULL,
  updated_at  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
)
```

## 비즈니스 규칙

1. **slug 생성**: title을 kebab-case로 변환 (예: "How To" → "how-to"). 현재 자동 생성 로직 미확인
2. **tagList**: PostgreSQL `text[]` 배열 타입 — `$1 = any(taglist)` 쿼리로 태그 필터링
3. **필터 우선순위**: tag → favorited → author 순서로 첫 번째 조건만 적용
4. **페이지네이션**: offset/limit이 0일 경우 전체 반환 (현재 구현)
5. **작성자 인증**: 생성 시 `LoginFilter`를 통과하나, 현재 구현에서 user_id 자동 설정 로직 미확인

## 구현 상태

| 항목 | 상태 | 비고 |
|------|------|------|
| POST /articles | 완료 | `ArticlesController::newArticle()` |
| GET /articles (전체) | 완료 | `ArticlesController::getAllArticles()` |
| GET /articles?tag= | 완료 | Raw SQL 쿼리 사용 |
| GET /articles?favorited= | 완료 | JOIN 쿼리 사용 |
| GET /articles?author= | 완료 | JOIN 쿼리 사용 |
| GET /articles/:slug | 미구현 | - |
| PUT /articles/:slug | 미구현 | - |
| DELETE /articles/:slug | 미구현 | - |
| GET /articles/feed | 미구현 | - |
| GET /tags | 미구현 | - |
| 목록 응답 상태코드 | 버그 | 201 반환 중, 200이어야 함 |
| articlesCount 필드 | 미구현 | RealWorld 스펙: `{"articles": [...], "articlesCount": N}` |
