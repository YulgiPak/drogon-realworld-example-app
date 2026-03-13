---
name: generate-plan
description: 프로젝트 바이브 코딩 전환 계획서(migration-plan.md) 생성 스킬. 코드베이스 분석 후 단계별 전환 계획을 작성한다.
---

# Vibe Coding Migration Plan Generator

## Phase 0: 코드베이스 자동 스캔
다음 항목을 자동으로 분석한다:
- 프로젝트 언어/프레임워크/빌드 시스템
- 현재 테스트 커버리지 및 테스트 존재 여부
- CI/CD 파이프라인 현황
- 문서화 수준 (README, API 문서 등)
- 기술 부채 지표 (TODO, FIXME, 버그)

## Phase 1: 심층 인터뷰 (3~5 라운드)
다음 주제로 사용자 인터뷰 진행:
1. 팀 구성 및 AI 도구 현재 사용 현황
2. 현재 개발 워크플로우의 병목 지점
3. 테스트 자동화 목표
4. CI/CD 자동화 목표
5. 우선순위 및 타임라인

## Phase 2: 분석 요약
인터뷰 결과를 요약하고 사용자 승인 요청

## Phase 3: migration-plan.md 생성
프로젝트 루트에 migration-plan.md 생성
