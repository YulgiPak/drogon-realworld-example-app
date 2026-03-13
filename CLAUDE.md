# CLAUDE.md

## 프로젝트 개요

[RealWorld](https://github.com/gothinkster/realworld) API 스펙을 C++20 + Drogon 프레임워크로 구현한 백엔드 서버.
Conduit 블로그 플랫폼 — 회원가입/로그인, 아티클 CRUD, 팔로우, 댓글, 즐겨찾기 기능 제공.

## 기술 스택

| 항목 | 내용 |
|------|------|
| **언어** | C++20 |
| **HTTP 프레임워크** | [Drogon](https://github.com/drogonframework/drogon) |
| **데이터베이스** | PostgreSQL |
| **인증** | JWT ([jwt-cpp](https://github.com/Thalhammer/jwt-cpp)) |
| **암호화** | bcrypt ([libbcrypt](https://github.com/trusch/libbcrypt)) |
| **빌드 시스템** | CMake (≥ 3.5) |
| **포트** | 3000 |

## 디렉토리 구조

```
.
├── CMakeLists.txt              ← 빌드 설정
├── config.json                 ← 런타임 설정 (DB, JWT, 포트)
├── main.cc                     ← 진입점
├── V1__create_tables.sql       ← DB 스키마 마이그레이션
├── controllers/                ← HTTP 컨트롤러 (라우팅 + 핸들러)
│   ├── api_ArticlesController  ← /articles 엔드포인트
│   └── api_UsersController     ← /users, /user 엔드포인트
├── filters/                    ← 요청 필터
│   └── LoginFilter             ← JWT 인증 필터 (인증 필요 라우트에 적용)
├── models/                     ← Drogon ORM 모델
│   ├── Users                   ← 사용자
│   ├── Articles                ← 아티클
│   ├── Comments                ← 댓글
│   ├── Follows                 ← 팔로우 관계
│   └── ArticleFavorites        ← 즐겨찾기
├── utils/
│   └── jwtService              ← JWT 생성/검증 유틸리티
└── external/                   ← 서드파티 의존성 (수정 금지)
    ├── jwt-cpp/
    └── libbcrypt/
```

## 빌드 방법

```bash
# 빌드 디렉토리 생성 및 빌드
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

빌드 산출물: `build/arslanTech`

## 실행 방법

```bash
# PostgreSQL DB 먼저 준비 후 실행
./build/arslanTech
```

서버는 `http://0.0.0.0:3000` 에서 수신 대기.

## 데이터베이스 설정

`config.json` 기준 기본값:

| 항목 | 값 |
|------|-----|
| host | 127.0.0.1 |
| port | 5432 |
| dbname | realworld |
| user | postgres |
| passwd | (없음) |

초기 스키마 적용:
```bash
psql -U postgres -d realworld -f V1__create_tables.sql
```

## API 엔드포인트

| Method | Path | 인증 | 설명 |
|--------|------|------|------|
| POST | /users | ❌ | 회원가입 |
| POST | /users/login | ❌ | 로그인 |
| GET | /user | ✅ | 현재 사용자 조회 |
| PUT | /user | ✅ | 사용자 정보 수정 |
| POST | /articles | ✅ | 아티클 생성 |
| GET | /articles | ❌ | 아티클 목록 조회 (tag/author/favorited/offset/limit 필터) |

인증은 `Authorization: Token <jwt>` 헤더로 전달. `LoginFilter`가 검증.

## 인증 흐름

1. `/users` 또는 `/users/login` 으로 JWT 토큰 발급
2. 이후 요청 헤더에 `Authorization: Token <jwt>` 포함
3. `LoginFilter`가 토큰 검증 → `jwtService::getCurrentUserIdFromRequest()` 로 user_id 추출

## 주요 설정 (config.json)

```json
"custom_config": {
  "jwt-secret": "<secret-key>",
  "jwt-sessionTime": 86400
}
```

JWT 시크릿은 `config.json`의 `custom_config.jwt-secret`에서 관리. 프로덕션 배포 시 반드시 변경.

## 코드 컨벤션

- 네임스페이스: `api` (컨트롤러), `drogon_model::realworld` (모델)
- 반환 타입 후치 (`auto foo() -> void`)
- Drogon ORM `Mapper<T>` 패턴으로 DB 접근
- 비동기 콜백 패턴: `function<void(const HttpResponsePtr &)> &&callback`

## 외부 의존성

| 라이브러리 | 용도 | 링크 방식 |
|-----------|------|---------|
| Drogon | HTTP 서버/ORM | find_package (시스템 설치 필요) |
| jwt-cpp | JWT 생성/검증 | add_subdirectory (external/) |
| libbcrypt | 비밀번호 해싱 | add_subdirectory (external/) |

Drogon은 시스템에 별도 설치 필요:
```bash
# Ubuntu/Debian
sudo apt install libdrogon-dev
# 또는 소스 빌드: https://github.com/drogonframework/drogon
```

## Git Hooks 설정

```bash
make setup-hooks
```

| Hook | 실행 시점 | 체크 항목 |
|------|-----------|-----------|
| pre-commit | git commit | 린트 + 단위 테스트 (make check) |
| pre-push | git push | E2E 테스트 (make e2e) |

## OpenSpec

`openspec/` 디렉토리에 기능별 스펙 문서 관리.
변경 제안 시 `/opsx:propose`, 구현 시 `/opsx:apply`, 완료 후 `/opsx:archive` 사용.
