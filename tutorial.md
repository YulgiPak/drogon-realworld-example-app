# 바이브 코딩 워크숍 튜토리얼: C++ Drogon 프로젝트에 AI 개발 워크플로우 적용하기

> **워크숍**: 다이렉트 클라우드랩 바이브 코딩 워크숍 (개발자 트랙)
> **날짜**: 2026-03-13
> **대상 프로젝트**: [drogon-realworld-example-app](https://github.com/YulgiPak/drogon-realworld-example-app) (arslan2012/drogon-realworld-example-app Fork)

---

## 1. 개요

이 튜토리얼은 기존 오픈소스 프로젝트에 Claude Code와 AI 도구를 활용하여 **문서화 → 테스트 → CI/CD** 파이프라인을 구축하는 전 과정을 다룹니다.

### 이 튜토리얼로 배우는 것

- **OpenSpec**으로 기존 코드베이스를 구조화된 스펙 문서로 자동 변환하는 방법
- **Claude 스킬**을 정의하여 반복 작업을 자동화하는 방법
- **Google Test** 단위 테스트를 AI가 생성하고 Human-in-the-loop으로 검증하는 방법
- **Git Hooks**로 로컬 품질 게이트(린트 + 테스트)를 구축하는 방법
- AI가 생성한 코드에서 실제 버그를 발견하고 수정하는 패턴

### 사용 환경

| 항목 | 내용 |
|------|------|
| GitHub | YulgiPak/drogon-realworld-example-app |
| 언어 | C++20 |
| 프레임워크 | Drogon |
| 도구 | Claude Code + OpenSpec + GitHub CLI |

### 워크숍 전후 비교

| 항목 | 워크숍 전 | 워크숍 후 |
|------|----------|----------|
| AI 컨텍스트 문서 | ❌ 없음 | ✅ CLAUDE.md + OpenSpec 7개 파일 |
| 단위 테스트 | ❌ 없음 | ✅ 46개 (Google Test) |
| 린트 설정 | ❌ 없음 | ✅ clang-tidy + clang-format |
| CI/CD | ❌ 없음 | ✅ Git Hooks (pre-commit + pre-push) |
| 빌드 자동화 | ❌ 없음 | ✅ Makefile (6개 타겟) |
| AI 스킬 | ❌ 없음 | ✅ 9개 Claude 스킬 |

---

## 2. 사전 준비

### 필수 도구 설치

```bash
# Claude Code 설치
npm install -g @anthropic/claude-code

# OpenSpec CLI 설치
npm install -g openspec-cli

# GitHub CLI 설치
# https://cli.github.com 에서 OS별 설치

# C++ 빌드 도구 (Ubuntu/Debian 기준)
sudo apt install build-essential cmake clang clang-tidy clang-format
sudo apt install libdrogon-dev libgtest-dev

# Google Test 빌드
cd /usr/src/gtest && sudo cmake . && sudo make && sudo cp lib/*.a /usr/lib
```

### 프로젝트 Fork 및 Clone

```bash
# GitHub에서 대상 프로젝트 Fork 후 Clone
git clone https://github.com/YulgiPak/drogon-realworld-example-app.git
cd drogon-realworld-example-app

# Claude Code 시작
claude
```

### 프로젝트 구조 파악

이 튜토리얼의 대상 프로젝트는 [RealWorld](https://github.com/gothinkster/realworld) API 스펙을 C++20 + Drogon으로 구현한 백엔드입니다.

```
.
├── controllers/     ← HTTP 컨트롤러 (라우팅 + 핸들러)
├── filters/         ← JWT 인증 필터
├── models/          ← Drogon ORM 모델
├── utils/           ← JWT 유틸리티
└── external/        ← 서드파티 의존성 (jwt-cpp, libbcrypt)
```

---

## 3. 1단계: 문서화

### 목표

AI가 프로젝트를 깊이 이해하도록 구조화된 컨텍스트 문서를 생성합니다.
컨텍스트가 충분하지 않으면 AI는 잘못된 가정을 하거나 환각(hallucination)을 일으킵니다.

### 3.1 OpenSpec 초기화

```bash
openspec init --tools claude
```

이 명령은 `openspec/project.md` (프로젝트 개요)와 `openspec/specs/` 디렉토리를 생성합니다.

### 3.2 CLAUDE.md 수동 생성

Claude Code가 프로젝트를 시작할 때마다 읽는 컨텍스트 파일입니다. AI에게 "이 파일을 바탕으로 작업해"라고 알려주는 역할을 합니다.

**프롬프트:**
```
이 프로젝트의 CLAUDE.md를 작성해줘.
controllers/, models/, utils/ 를 분석하고 다음을 포함해:
- 프로젝트 개요와 기술 스택
- 디렉토리 구조와 각 디렉토리의 역할
- 빌드 방법, 실행 방법
- API 엔드포인트 목록
- 인증 흐름
- 코드 컨벤션
```

**생성된 항목:**
- `CLAUDE.md` — Claude Code용 프로젝트 컨텍스트 파일

### 3.3 GitHub 이슈 등록

실제로 발견된 문제와 개선 사항을 이슈로 관리합니다.

```bash
# 이슈 1: 테스트 없음
gh issue create \
  --title "단위 테스트 없음 — Google Test 도입 필요" \
  --body "JWT 서비스, bcrypt 해싱, 입력값 검증 로직에 대한 단위 테스트가 없습니다."

# 이슈 2: 린트 설정 없음
gh issue create \
  --title "clang-tidy / clang-format 설정 없음" \
  --body "코드 품질 게이트가 없어 일관성이 떨어집니다."

# 이슈 3: CI/CD 없음
gh issue create \
  --title "Git Hooks 기반 로컬 CI/CD 구축 필요" \
  --body "pre-commit: lint + test, pre-push: E2E 테스트"

# 이슈 4: 버그 — PUT /user setEmail 오타
gh issue create \
  --title "[BUG] PUT /user: 모든 필드에 setEmail() 호출 (오타)" \
  --body "api_UsersController.cc의 updateUser에서 username, bio, image 업데이트 시 setEmail()이 잘못 사용됨."
```

### 3.4 OpenSpec 스펙 파일 생성

**프롬프트:**
```
OpenSpec을 사용해서 현재 프로젝트를 문서화 해줘.
각각의 파일은 200라인을 넘지 않도록 나눠서 작성해줘.
기능별로 분리해: authentication, users, articles, comments, follows, article-favorites
```

**생성된 항목:**
- `openspec/project.md` — 프로젝트 전체 개요
- `openspec/specs/authentication/spec.md` — 로그인/회원가입 스펙
- `openspec/specs/users/spec.md` — 사용자 관리 스펙
- `openspec/specs/articles/spec.md` — 아티클 CRUD 스펙
- `openspec/specs/comments/spec.md` — 댓글 스펙
- `openspec/specs/follows/spec.md` — 팔로우 스펙
- `openspec/specs/article-favorites/spec.md` — 즐겨찾기 스펙

총 7개 파일로 프로젝트 전체 API 행동을 명세화합니다.

### 3.5 Claude 스킬 생성

**프롬프트:**
```
이 문서들을 활용해서 프로젝트에 유용한 스킬을 만들어줘.
반복적으로 사용할 개발 작업들을 스킬로 정의해.
```

**생성된 항목 (`.claude/skills/` — 9개):**

| 스킬 | 설명 |
|------|------|
| `openspec-propose` | 변경 제안 생성 |
| `openspec-apply-change` | 제안된 변경 적용 |
| `openspec-archive-change` | 완료된 변경 아카이브 |
| `openspec-explore` | 스펙 탐색 및 분석 |
| `implement-endpoint` | 새 API 엔드포인트 구현 |
| `write-unit-test` | 단위 테스트 작성 |
| `fix-realworld-bug` | RealWorld 버그 수정 |
| `db-migration` | DB 마이그레이션 생성 |
| `generate-plan` | 전환 계획서 생성 |

---

## 4. 2단계: 테스트 구현

### 목표

AI가 생성한 테스트를 검토하고 검증하는 **Human-in-the-loop** 패턴을 실습합니다.
AI가 생성한 코드를 맹목적으로 믿지 않고, 실제로 실행하고 결과를 확인합니다.

### 4.1 린트 설정

**프롬프트:**
```
C++20 Drogon 프로젝트에 맞는 clang-tidy와 clang-format 설정을 추가해줘.
외부 라이브러리(external/)는 제외하고, 프로젝트 코드만 검사하도록 설정해.
```

**생성된 항목:**
- `.clang-tidy` — 정적 분석 규칙 (modernize-*, readability-*, clang-analyzer-*)
- `.clang-format` — 코드 스타일 (Google 스타일 기반)

### 4.2 단위 테스트 작성

**프롬프트:**
```
Google Test를 사용해서 단위 테스트를 작성해줘.
OpenSpec으로 생성된 핵심 요구사항을 커버해야 해.
tests/ 디렉토리에 기능별로 파일을 분리해서 작성해줘.
```

**생성된 항목 (총 46개 테스트):**

`tests/test_jwtService.cc` — JWT 서비스 테스트
```
- JWT 토큰 생성 및 파싱 검증
- 만료된 토큰 처리
- 잘못된 시크릿 키로 검증 시 예외 처리
- 페이로드 필드 정확성 검증
```

`tests/test_bcrypt.cc` — bcrypt 암호화 테스트
```
- 비밀번호 해싱 검증
- 동일 비밀번호로 생성된 두 해시가 다름을 확인 (salt 검증)
- 잘못된 비밀번호로 검증 시 실패 확인
- 빈 비밀번호 처리
```

`tests/test_input_validation.cc` — 입력값 검증 테스트
```
- 이메일 형식 검증 (유효/무효 케이스)
- 비밀번호 최소 길이 검증
- 빈 필수 필드 처리
- SQL 인젝션 시도 거부
```

### 4.3 CMakeLists.txt 업데이트

**프롬프트:**
```
CMakeLists.txt에 tests/ 서브디렉토리를 연결하고
Google Test와 연동되도록 수정해줘.
```

```cmake
# CMakeLists.txt에 추가
enable_testing()
add_subdirectory(tests)
```

`tests/CMakeLists.txt`도 함께 생성하여 각 테스트 파일을 빌드 타겟으로 등록합니다.

### 4.4 Makefile 작성

**프롬프트:**
```
build/test/lint/format/check/e2e/clean 타겟을 가진 Makefile을 작성해줘.
check는 lint + test를 순서대로 실행하고, 하나라도 실패하면 중단해야 해.
```

**생성된 항목:**
- `Makefile` — 6개 타겟 (build, test, lint, format, check, e2e)

```bash
make build   # CMake Release 빌드
make test    # 단위 테스트 실행
make lint    # clang-tidy 정적 분석
make format  # clang-format 자동 포맷 적용
make check   # lint + test (pre-commit hook 용)
make e2e     # E2E 테스트 실행
make clean   # 빌드 산출물 삭제
```

---

## 5. 3단계: CI/CD 구축

### 목표

서버(GitHub Actions 등)가 없어도 로컬에서 코드 품질을 자동으로 검증하는 Git Hooks를 구축합니다.

### 5.1 Git Hooks 설정

**프롬프트:**
```
pre-commit hook을 설정해줘.
커밋할 때마다 린트 체크와 단위 테스트가 자동 실행되도록 해줘.
pre-push hook은 E2E 테스트가 실행되도록 해줘.
팀원들이 쉽게 설정할 수 있도록 setup 스크립트도 만들어줘.
```

**생성된 항목:**

`.githooks/pre-commit`:
```bash
#!/bin/sh
# 커밋 전 lint + 단위 테스트 실행
make check
```

`.githooks/pre-push`:
```bash
#!/bin/sh
# push 전 E2E 테스트 실행
make e2e
```

`scripts/setup-hooks.sh`:
```bash
#!/bin/sh
git config core.hooksPath .githooks
chmod +x .githooks/pre-commit .githooks/pre-push
echo "Git hooks 설정 완료"
```

### 5.2 Git Hooks 활성화

```bash
# Git hooks 디렉토리 등록
make setup-hooks
# 또는
sh scripts/setup-hooks.sh
```

이후 `git commit` 실행 시마다 자동으로 `make check` (lint + test)가 실행됩니다.

### 5.3 전환 계획서 생성

**프롬프트:**
```
/generate-plan
```

Claude의 `generate-plan` 스킬을 실행하여 팀 전체에 공유할 수 있는 바이브 코딩 전환 계획서(`migration-plan.md`)를 자동 생성합니다.

---

## 6. 발견된 버그와 대응 방법 (Human-in-the-loop 패턴)

AI가 코드를 분석하는 과정에서 발견된 실제 버그들입니다.
**AI를 맹신하지 말고, 항상 결과를 검토하는 습관**이 중요합니다.

### 버그 1: PUT /user — setEmail() 오타

**위치**: `controllers/api_UsersController.cc` — `updateUser` 함수

**문제**:
```cpp
// 버그: username, bio, image 업데이트 시 모두 setEmail()을 호출
if (!username.empty()) user.setEmail(username);  // ← setUsername() 이어야 함
if (!bio.empty())      user.setEmail(bio);        // ← setBio() 이어야 함
if (!image.empty())    user.setEmail(image);      // ← setImage() 이어야 함
```

**수정**:
```cpp
if (!username.empty()) user.setUsername(username);
if (!bio.empty())      user.setBio(bio);
if (!image.empty())    user.setImage(image);
```

**발견 방법**: OpenSpec의 사용자 스펙과 실제 구현 코드를 AI가 대조 분석하면서 발견.

### 버그 2: GET /articles — 잘못된 HTTP 상태 코드

**위치**: `controllers/api_ArticlesController.cc` — `getArticles` 함수

**문제**:
```cpp
// 버그: 조회 응답에 201 Created 반환 (4개 분기 모두)
auto resp = HttpResponse::newHttpJsonResponse(result);
resp->setStatusCode(k201Created);  // ← k200OK 이어야 함
```

**수정**:
```cpp
resp->setStatusCode(k200OK);
```

**발견 방법**: RealWorld API 스펙(GET은 200 OK, POST는 201 Created)과 구현을 비교하면서 발견.

### 버그 3: follows 테이블 — UNIQUE 제약 없음

**위치**: `V1__create_tables.sql`

**문제**:
```sql
-- 버그: 동일 팔로우 관계를 중복으로 삽입 가능
CREATE TABLE follows (
  follower_id INTEGER,
  following_id INTEGER
  -- UNIQUE 제약 없음!
);
```

**수정**:
```sql
CREATE TABLE follows (
  follower_id INTEGER,
  following_id INTEGER,
  UNIQUE(follower_id, following_id)  -- 중복 팔로우 방지
);
```

**발견 방법**: AI가 follows 스펙을 작성하면서 데이터 무결성 문제를 지적.

### Human-in-the-loop 체크리스트

AI가 코드를 생성하거나 분석할 때마다 다음을 확인하세요:

- [ ] 생성된 테스트가 실제로 실행되는가? (`make test`)
- [ ] 린트가 통과하는가? (`make lint`)
- [ ] API 응답 코드가 스펙과 일치하는가?
- [ ] DB 스키마에 필요한 제약 조건이 있는가?
- [ ] 타입 오타나 잘못된 메서드 호출은 없는가?

---

## 7. 다른 프로젝트에 적용하는 팁

### 적용 순서

프로젝트 규모나 언어에 관계없이 동일한 3단계 접근법을 사용하세요:

```
문서화 먼저 → 테스트로 행동 고정 → CI/CD로 자동 검증
```

### 프롬프트 패턴

**문서화 프롬프트 템플릿:**
```
이 프로젝트를 분석하고 CLAUDE.md를 작성해줘.
포함할 내용: 기술 스택, 디렉토리 구조, 빌드 방법, 주요 컴포넌트 설명
```

**테스트 프롬프트 템플릿:**
```
[테스트 프레임워크]를 사용해서 [모듈/파일]에 대한 단위 테스트를 작성해줘.
[OpenSpec/CLAUDE.md]의 요구사항을 커버해야 해.
```

**CI/CD 프롬프트 템플릿:**
```
pre-commit hook을 설정해줘.
커밋 전에 [린트 도구]와 단위 테스트가 자동 실행되도록 해줘.
```

### 언어별 대응 도구

| 언어 | 테스트 | 린트 | 포맷 |
|------|--------|------|------|
| C++ | Google Test | clang-tidy | clang-format |
| Go | testing + testify | golangci-lint | gofmt |
| Python | pytest | ruff / flake8 | black |
| TypeScript | Jest / Vitest | ESLint | Prettier |
| Java | JUnit 5 | Checkstyle | google-java-format |

### AI 활용 효율화 팁

1. **CLAUDE.md는 항상 최신 상태 유지** — 코드 변경 시 함께 업데이트
2. **스킬을 먼저 정의** — 반복 작업은 스킬로 만들어두면 "스킬명"만으로 실행 가능
3. **OpenSpec 스펙 200라인 제한** — 파일이 크면 AI가 전체 컨텍스트를 처리하기 어려움
4. **버그 발견 즉시 이슈 등록** — `gh issue create`로 트래킹
5. **AI 제안은 항상 실행 후 검증** — 실행하지 않은 테스트는 테스트가 아님

### 경고: AI 환각 방지

- AI가 생성한 API 응답 코드, 메서드 이름, DB 스키마는 **반드시 실제 코드와 대조**
- 특히 비슷한 이름의 메서드 (setEmail vs setUsername) 오타에 주의
- 스펙 문서 없이 테스트를 생성하면 AI가 잘못된 동작을 정상으로 테스트할 수 있음

---

## 참고 자료

- [RealWorld API 스펙](https://github.com/gothinkster/realworld)
- [Drogon 공식 문서](https://github.com/drogonframework/drogon)
- [OpenSpec CLI](https://openspec.dev)
- [Claude Code 문서](https://claude.ai/code)
- [Google Test 가이드](https://google.github.io/googletest/)
