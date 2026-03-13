#!/bin/sh
# setup-hooks.sh: .githooks/ 를 Git hooks 디렉토리로 설정

git config core.hooksPath .githooks
chmod +x .githooks/pre-commit
chmod +x .githooks/pre-push
echo "Git hooks 설정 완료"
echo "  core.hooksPath = .githooks"
echo "  .githooks/pre-commit  (린트 + 단위 테스트)"
echo "  .githooks/pre-push    (E2E 테스트)"
