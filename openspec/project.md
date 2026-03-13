# Drogon RealWorld Example App — 프로젝트 컨텍스트

## 프로젝트 개요

[RealWorld](https://github.com/gothinkster/realworld) 스펙을 C++20 + Drogon 프레임워크로 구현한 백엔드 API 서버.
Medium.com 클론 앱(Conduit)의 REST API를 제공한다.

## 기술 스택

| 항목 | 내용 |
|------|------|
| 언어 | C++20 |
| 프레임워크 | Drogon (비동기 HTTP 서버) |
| 데이터베이스 | PostgreSQL |
| 인증 | JWT (jwt-cpp) + bcrypt (libbcrypt) |
| 빌드 | CMake |
| DB 마이그레이션 | V1__create_tables.sql |

## 디렉토리 구조

```
drogon-realworld-example-app/
├── controllers/
│   ├── api_UsersController.h/.cc   ← 사용자 관련 API
│   └── api_ArticlesController.h/.cc ← 아티클 관련 API
├── filters/
│   └── LoginFilter.h/.cc           ← JWT 인증 필터
├── models/
│   ├── Users                       ← Drogon ORM 모델
│   ├── Articles
│   ├── Comments
│   ├── Follows
│   └── ArticleFavorites
├── utils/
│   └── jwtService.h/.cc            ← JWT 생성/검증 서비스
├── external/
│   └── libbcrypt/                  ← bcrypt 라이브러리
├── openspec/
│   ├── project.md                  ← 이 파일
│   └── specs/                      ← 기능별 API 스펙
├── V1__create_tables.sql           ← DB 스키마
└── config.json                     ← Drogon 서버 설정
```

## 인증 방식

- **방식**: Bearer Token (JWT)
- **헤더**: `Authorization: Token <jwt_token>`
- **토큰 생성**: 로그인/회원가입 성공 시 발급
- **토큰 검증**: `LoginFilter`가 보호된 엔드포인트에서 자동 처리
- **페이로드**: user_id 포함

## 데이터베이스 스키마 요약

```sql
users           (id, username, email, password, bio, image)
articles        (id, user_id, slug, title, description, body, tagList text[], created_at, updated_at)
article_favorites (article_id, user_id)  -- PK 복합키
follows         (user_id, follow_id)
comments        (id, body, article_id, user_id, created_at, updated_at)
```

## 공통 응답 규칙

- **Content-Type**: `application/json`
- **인증 필요 엔드포인트**: `Authorization: Token <jwt>` 헤더 필수
- **에러 응답 형식**:
  ```json
  { "errors": { "body": ["에러 메시지"] } }
  ```
- **날짜 형식**: ISO 8601 (`2024-01-01T00:00:00.000Z`)

## 구현 현황

| 기능 | 상태 |
|------|------|
| 회원가입 (POST /users) | 구현 완료 |
| 로그인 (POST /users/login) | 구현 완료 |
| 현재 사용자 조회 (GET /user) | 구현 완료 |
| 사용자 정보 수정 (PUT /user) | 구현 완료 (버그 있음) |
| 아티클 생성 (POST /articles) | 구현 완료 |
| 아티클 목록 조회 (GET /articles) | 구현 완료 |
| 아티클 상세/수정/삭제 | 미구현 |
| 댓글 CRUD | 미구현 |
| 팔로우/언팔로우 | 미구현 |
| 즐겨찾기 | 미구현 |
| 피드 (GET /articles/feed) | 미구현 |
| 태그 목록 (GET /tags) | 미구현 |
| 프로필 조회 (GET /profiles/:username) | 미구현 |

## 알려진 버그

- `PUT /user`: `update()` 구현에서 모든 필드 수정 시 `setEmail()`만 호출하는 버그 존재
  (username, password, bio, image 수정이 실제로는 email 필드에 덮어씌워짐)

## 참고 링크

- [RealWorld API 스펙](https://realworld-docs.netlify.app/docs/specs/backend-specs/endpoints)
- [RealWorld GitHub](https://github.com/gothinkster/realworld)
- [Drogon 프레임워크](https://github.com/drogonframework/drogon)
