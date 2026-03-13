# Article Favorites Spec

## 개요

아티클 즐겨찾기(좋아요) 추가 및 해제 API.
두 엔드포인트 모두 인증 필요.
현재 미구현 상태이며, GET /articles?favorited= 필터는 이미 구현되어 있다.

## API 엔드포인트

### POST /articles/:slug/favorite — 즐겨찾기 추가

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
POST /articles/how-to-train-your-dragon/favorite
Authorization: Token jwt.token.here
```

Request body 없음.

#### Response 200 OK

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
    "favorited": true,
    "favoritesCount": 1,
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

### DELETE /articles/:slug/favorite — 즐겨찾기 해제

**인증 필요** (`Authorization: Token <jwt>`)

#### Request

```http
DELETE /articles/how-to-train-your-dragon/favorite
Authorization: Token jwt.token.here
```

#### Response 200 OK

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
    "favorited": false,
    "favoritesCount": 0,
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

- 존재하지 않는 slug 또는 즐겨찾기 관계 없음

---

## 데이터 모델

```sql
article_favorites (
  article_id integer not null references articles,
  user_id    integer not null references users,
  primary key (article_id, user_id)   -- 복합 PK로 중복 방지
)
```

### Article 응답 객체의 즐겨찾기 관련 필드

| 필드 | 타입 | 설명 |
|------|------|------|
| favorited | boolean | 현재 인증 사용자의 즐겨찾기 여부 |
| favoritesCount | integer | 총 즐겨찾기 수 |

### favoritesCount 계산

```sql
SELECT COUNT(*) FROM article_favorites WHERE article_id = :articleId
```

### favorited 계산

```sql
SELECT EXISTS (
  SELECT 1 FROM article_favorites
  WHERE article_id = :articleId AND user_id = :currentUserId
) AS favorited
```

## 비즈니스 규칙

1. **중복 방지**: `article_favorites`에 복합 PK `(article_id, user_id)` — 동일 사용자의 중복 즐겨찾기 불가
2. **slug → article_id 변환**: 경로 파라미터 slug로 articles 테이블 조회
3. **favorited 필드**: 비인증 요청 시 항상 `false`; 인증 시 article_favorites 테이블에서 조회
4. **favoritesCount**: 즐겨찾기 추가/해제 후 재계산하여 응답에 포함

## 구현 상태

| 항목 | 상태 | 비고 |
|------|------|------|
| POST /articles/:slug/favorite | 미구현 | ArticleFavorites 모델은 존재 |
| DELETE /articles/:slug/favorite | 미구현 | - |
| GET /articles?favorited= 필터 | 구현 완료 | `ArticlesController::getAllArticles()` |

### 구현 시 필요한 작업

1. `api_ArticleFavoritesController.h/.cc` 생성 (또는 ArticlesController에 확장)
2. slug → article_id 조회 헬퍼 (Comments 구현과 공유 가능)
3. `article_favorites` Drogon ORM 모델 확인
4. favoritesCount 집계 쿼리
5. 아티클 응답 객체에 `favorited`, `favoritesCount` 필드 추가

### 기존 GET /articles?favorited= 쿼리 참고

```sql
SELECT a.id, a.user_id, a.slug, a.title, a.description, a.body, a.created_at, a.updated_at
FROM articles a
INNER JOIN article_favorites af ON a.id = af.article_id
INNER JOIN users u ON af.user_id = u.id
WHERE u.username = $1
```
