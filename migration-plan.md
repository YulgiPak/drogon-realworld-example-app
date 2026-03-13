# Vibe Coding 전환 계획서: drogon-realworld-example-app

> 작성일: 2026-03-13
> 대상 프로젝트: [drogon-realworld-example-app](https://github.com/gothinkster/realworld) (C++20 / Drogon / PostgreSQL)
> 워크숍: 다이렉트 클라우드랩 바이브 코딩 워크숍 (개발자 트랙)

---

## 1. 현황 분석

### 기술 스택
| 항목 | 내용 |
|------|------|
| 언어 | C++20 |
| 프레임워크 | Drogon |
| 빌드 시스템 | CMake |
| 데이터베이스 | PostgreSQL |
| 테스트 프레임워크 | Google Test |
| AI 도구 | Claude Code (YOLO 모드) |

### 워크숍 전 기준선
| 항목 | 워크숍 전 상태 |
|------|--------------|
| 테스트 | 없음 |
| 린트 | 없음 |
| CI/CD | 없음 |
| AI 컨텍스트 문서 | 없음 |
| API 사양 문서 | 없음 |

### 발견된 버그 (코드베이스 스캔)
1. **PUT /user setter 오타** — 필드명 불일치로 업데이트 누락 가능
2. **GET /articles 상태코드 버그** — 성공 시 잘못된 HTTP 상태코드 반환
3. **follows 테이블 UNIQUE 제약 없음** — 중복 팔로우 레코드 생성 가능

---

## 2. 워크숍 완료 항목

워크숍(2026-03-13) 중 아래 기반 작업이 완료되었다.

### 2-1. 문서화 (1단계)
- [x] **CLAUDE.md 생성** — AI 협업을 위한 프로젝트 컨텍스트 문서. 아키텍처, 코딩 컨벤션, 자주 쓰는 커맨드 포함
- [x] **OpenSpec 스펙 파일 7개 생성** — 각 API 엔드포인트별 사양 문서 (openspec/ 디렉토리)
  - users-register, users-login, user-get, user-update
  - profiles, articles, comments
- [x] **Claude 스킬 8개 생성** (.claude/skills/)
  - `openspec-explore`, `openspec-propose`, `openspec-apply-change`, `openspec-archive-change`
  - `implement-endpoint`, `write-unit-test`, `fix-realworld-bug`, `db-migration`

### 2-2. 테스트 구현 (2단계)
- [x] **Google Test 단위 테스트 46개** (tests/ 디렉토리)
  - 각 컨트롤러/유틸리티 함수 대상
  - 경계값, 오류 케이스, 정상 케이스 포함
- [x] **clang-tidy 설정** — 정적 분석 규칙 정의
- [x] **clang-format 설정** — 코드 스타일 자동 포매팅

### 2-3. CI/CD 구축 (3단계)
- [x] **Makefile 타깃 정의** — `build`, `test`, `lint`, `check`, `e2e`
- [x] **pre-commit Hook** — 커밋 전 clang-format + clang-tidy 자동 실행
- [x] **pre-push Hook** — 푸시 전 전체 테스트 스위트 자동 실행

---

## 3. 단기 계획 (1~2주)

### 목표: 발견된 버그 수정 및 테스트 안정화

#### 액션 아이템
| # | 작업 | 담당 | 성공 기준 |
|---|------|------|----------|
| 1 | PUT /user setter 오타 수정 | 개발자 | 단위 테스트 통과 + E2E 확인 |
| 2 | GET /articles 상태코드 수정 | 개발자 | RealWorld API 스펙 준수 확인 |
| 3 | follows 테이블 UNIQUE 제약 추가 | 개발자 | DB 마이그레이션 스크립트 작성 + 적용 |
| 4 | 기존 테스트 46개 안정화 | 개발자 | CI에서 100% 통과 |
| 5 | Claude Code YOLO 모드 팀 공유 | 팀 리드 | 팀원 전원 로컬 실행 확인 |

#### 사용 스킬
- `fix-realworld-bug` — 버그 수정 가이드
- `db-migration` — UNIQUE 제약 마이그레이션
- `write-unit-test` — 수정 후 회귀 테스트 추가

---

## 4. 중기 계획 (1개월)

### 목표: 테스트 커버리지 확대 + GitHub Actions CI 구축

#### 액션 아이템
| # | 작업 | 담당 | 성공 기준 |
|---|------|------|----------|
| 1 | 통합 테스트 추가 (E2E) | 개발자 | RealWorld Postman 컬렉션 100% 통과 |
| 2 | GitHub Actions 워크플로우 생성 | 개발자/DevOps | PR마다 빌드+테스트 자동 실행 |
| 3 | 코드 커버리지 리포트 연동 | 개발자 | lcov 또는 gcovr 리포트 CI 업로드 |
| 4 | OpenSpec 기반 API 변경 프로세스 정착 | 팀 리드 | 모든 API 변경이 OpenSpec → 구현 순서로 진행 |
| 5 | 신규 기능 개발 시 `implement-endpoint` 스킬 적용 | 개발자 전원 | 스킬 사용 로그 확인 |

#### GitHub Actions 예시 구조
```yaml
# .github/workflows/ci.yml
on: [push, pull_request]
jobs:
  build-and-test:
    steps:
      - cmake build
      - make test
      - make lint
      - 커버리지 업로드
```

---

## 5. 장기 계획 (3개월)

### 목표: 바이브 코딩 완전 정착 + 팀 생산성 지표 측정

#### 액션 아이템
| # | 작업 | 담당 | 성공 기준 |
|---|------|------|----------|
| 1 | CD (Continuous Deployment) 파이프라인 구축 | DevOps | main 머지 시 스테이징 자동 배포 |
| 2 | 테스트 커버리지 목표 달성 | 개발자 | 라인 커버리지 80% 이상 |
| 3 | Claude Code 사용 로그 기반 튜토리얼 작성 | 팀 리드 | 사내 위키 또는 GitHub 문서 게시 |
| 4 | 다른 C++ 프로젝트에 동일 패턴 적용 | 팀 전체 | 최소 1개 추가 프로젝트 전환 완료 |
| 5 | 분기별 생산성 회고 | 팀 리드 | 버그 발생률·PR 사이클 타임 비교 |

#### 측정 지표
- **버그 발생률**: 워크숍 전 대비 감소율
- **PR 사이클 타임**: 코드 작성 → 머지까지 소요 시간
- **테스트 작성 시간**: AI 보조 전후 비교
- **코드 리뷰 코멘트 수**: 린트/포맷 관련 코멘트 감소 여부

---

## 6. 팀 적용 가이드

### 일상적인 바이브 코딩 워크플로우

```
1. 기능 요청 수신
   └─ openspec-propose 스킬로 OpenSpec 초안 작성
       └─ openspec-apply-change 스킬로 사양 확정
           └─ implement-endpoint 스킬로 구현
               └─ write-unit-test 스킬로 테스트 추가
                   └─ git commit → pre-commit hook 자동 실행
                       └─ git push → pre-push hook (테스트 전체 실행)
```

### Claude Code 사용 원칙
1. **CLAUDE.md 먼저** — 새 세션 시작 시 Claude가 CLAUDE.md를 자동으로 읽는다
2. **스킬 활용** — 반복 작업은 `.claude/skills/` 스킬로 위임
3. **Human-in-the-loop** — AI 생성 코드는 반드시 사람이 검토 후 커밋
4. **OpenSpec 우선** — 구현 전 스펙 문서 작성을 원칙으로 한다
5. **YOLO 모드 주의** — 자동 실행 범위를 팀 내 합의 후 설정

### 신규 팀원 온보딩 체크리스트
- [ ] Claude Code 설치 및 라이선스 확인
- [ ] CLAUDE.md 읽기
- [ ] `.claude/skills/` 스킬 목록 숙지
- [ ] `make check` 로컬 실행 확인 (빌드 + 린트 + 테스트)
- [ ] pre-commit/pre-push hook 동작 확인
- [ ] OpenSpec 파일 구조 파악 (openspec/ 디렉토리)

---

## 7. 예상 효과

| 지표 | 현재 | 1개월 후 목표 | 3개월 후 목표 |
|------|------|-------------|-------------|
| 단위 테스트 수 | 46개 | 80개 이상 | 150개 이상 |
| 테스트 커버리지 | 측정 전 | 50% 이상 | 80% 이상 |
| 버그 발견 시점 | 운영 환경 | PR 단계 | 로컬 커밋 단계 |
| API 문서화 | 7개 스펙 | 전 엔드포인트 | 자동 생성 연동 |
| CI 자동화 | 로컬 Hooks | GitHub Actions | CD 포함 전체 자동화 |
| 코드 리뷰 효율 | 기준 없음 | 포맷 이슈 제로 | 사이클 타임 30% 단축 |

---

## 부록: 참고 자료

- [RealWorld API Spec](https://realworld-docs.netlify.app/docs/specs/backend-specs/introduction)
- [Drogon 공식 문서](https://drogon.docsforge.com/)
- [Google Test 가이드](https://google.github.io/googletest/)
- [Claude Code 공식 문서](https://docs.anthropic.com/claude/docs/claude-code)
- 워크숍 학생 가이드: `vibecoding-for-developer/student-guide.md`
