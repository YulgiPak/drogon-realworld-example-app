---
name: db-migration
description: PostgreSQL 마이그레이션 스크립트 작성 및 Drogon ORM 모델 업데이트. 새 테이블/컬럼 추가, 스키마 변경 시 사용.
---

PostgreSQL 마이그레이션 스크립트를 작성하고 Drogon ORM 모델을 업데이트한다.

## 1단계: 현재 스키마 파악

```bash
cat V1__create_tables.sql
```

현재 테이블 구조:
```sql
users          (id, username, password, email, bio, image)
articles       (id, user_id, slug, title, description, body, tagList text[], created_at, updated_at)
article_favorites (article_id, user_id)  -- PK: (article_id, user_id)
follows        (user_id, follow_id)
comments       (id, body, article_id, user_id, created_at, updated_at)
```

기존 마이그레이션 파일 목록 확인:
```bash
ls V*__*.sql
```

## 2단계: 마이그레이션 파일 명명 규칙

```
V<버전>__<설명>.sql
```

예시:
- `V1__create_tables.sql` (기존)
- `V2__add_token_blacklist.sql`
- `V3__add_article_tags_table.sql`

규칙:
- 버전 번호는 순차적으로 증가
- 설명은 snake_case, 영문
- 동일 버전 번호 사용 금지 (배포 후 되돌릴 수 없음)

## 3단계: 마이그레이션 스크립트 작성

### 새 테이블 추가 패턴
```sql
-- V2__add_<table_name>.sql

CREATE TABLE <table_name> (
    id         serial primary key,
    -- 외래키는 항상 references 명시
    user_id    integer not null references users,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- 인덱스 (JOIN/WHERE에 자주 사용되는 컬럼)
CREATE INDEX idx_<table>_<column> ON <table_name>(<column>);
```

### 컬럼 추가 패턴
```sql
-- V2__add_<column>_to_<table>.sql

ALTER TABLE <table_name>
    ADD COLUMN <column_name> <type> [NOT NULL] [DEFAULT <value>];
```

### 롤백 안전 패턴 (옵션)
```sql
-- 마이그레이션 실패 시 롤백 가능하도록 트랜잭션 사용
BEGIN;

ALTER TABLE articles ADD COLUMN reading_time integer;

-- 검증
DO $$ BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_name='articles' AND column_name='reading_time'
    ) THEN RAISE EXCEPTION 'Migration failed'; END IF;
END $$;

COMMIT;
```

## 4단계: Drogon ORM 모델 재생성

스키마 변경 후 모델 파일을 재생성한다.

```bash
# config.json에서 DB 연결 설정 확인
cat config.json

# drogon_ctl로 모델 재생성
drogon_ctl create model models/

# 또는 특정 테이블만 재생성
drogon_ctl create model models/ -t <table_name>
```

재생성 후 변경된 파일 확인:
```bash
ls models/
# 예: models/Articles.h, models/Articles.cc, models/Users.h 등
```

## 5단계: 마이그레이션 적용

```bash
# Flyway (프로젝트에 설정된 경우)
flyway migrate

# 또는 psql 직접 실행
psql -U <user> -d <database> -f V2__<description>.sql

# 적용 확인
psql -U <user> -d <database> -c "\d <table_name>"
```

## 주의사항

- **되돌릴 수 없는 변경**: `DROP TABLE`, `DROP COLUMN`은 데이터 손실 위험 — 운영 적용 전 반드시 백업
- **NOT NULL 컬럼 추가**: 기존 데이터가 있으면 `DEFAULT` 값 없이 `NOT NULL` 추가 불가
- **tagList 배열 타입**: `text[]` — 일반 ORM Mapper가 아닌 Raw SQL 필요할 수 있음 (`$1 = any(taglist)` 패턴)
- **모델 재생성 후**: 기존 `.h`/`.cc`에 직접 수정한 코드가 있다면 재생성으로 덮어써짐 — 커스텀 로직은 별도 유틸 클래스로 분리

## 현재 스키마 확장 예시

RealWorld 스펙에서 추가 가능한 마이그레이션:

```sql
-- V2__add_follows_primary_key.sql
-- follows 테이블에 복합 PK 추가 (현재 없음)
ALTER TABLE follows ADD PRIMARY KEY (user_id, follow_id);

-- V3__add_articles_count_index.sql
-- 성능 최적화: 자주 조회되는 컬럼 인덱싱
CREATE INDEX idx_articles_user_id ON articles(user_id);
CREATE INDEX idx_articles_created_at ON articles(created_at DESC);
CREATE INDEX idx_comments_article_id ON comments(article_id);
```
