#!/bin/bash
# tests/e2e/test_api.sh — RealWorld API E2E 테스트
#
# 실행 방법:
#   bash tests/e2e/test_api.sh
#   또는 make e2e (서버가 실행 중이어야 함)
#
# 서버가 없으면 "서버 미실행 — E2E 테스트 건너뜀" 후 exit 0

set -euo pipefail

# ── 설정 ──────────────────────────────────────────────────────────────────────

BASE_URL="${REALWORLD_BASE_URL:-http://localhost:3000}"
API_URL="${BASE_URL}/api"

# 테스트용 고유 사용자 (타임스탬프 기반)
TS=$(date +%s)
TEST_EMAIL="e2e_${TS}@example.com"
TEST_USERNAME="e2euser_${TS}"
TEST_PASSWORD="Password123!"

PASS=0
FAIL=0
SKIP=0

# ── 색상 출력 ─────────────────────────────────────────────────────────────────

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ok()   { echo -e "  ${GREEN}[PASS]${NC} $1"; PASS=$((PASS + 1)); }
fail() { echo -e "  ${RED}[FAIL]${NC} $1"; FAIL=$((FAIL + 1)); }
info() { echo -e "  ${YELLOW}[INFO]${NC} $1"; }

# ── 서버 연결 확인 ────────────────────────────────────────────────────────────

echo ""
echo "=== RealWorld API E2E 테스트 ==="
echo "대상 서버: ${API_URL}"
echo ""

if ! curl -s --max-time 3 "${BASE_URL}" > /dev/null 2>&1; then
    echo -e "${YELLOW}서버 미실행 — E2E 테스트 건너뜀${NC}"
    echo "서버를 시작한 후 다시 실행하세요: ./build/arslanTech"
    exit 0
fi

info "서버 응답 확인 완료"

# ── 헬퍼 함수 ─────────────────────────────────────────────────────────────────

# HTTP 상태 코드만 추출
http_status() {
    curl -s -o /dev/null -w "%{http_code}" "$@"
}

# 응답 본문 + 상태 코드 함께 추출
http_response() {
    curl -s -w "\n__STATUS__%{http_code}" "$@"
}

# 응답에서 상태 코드 파싱
parse_status() {
    echo "$1" | grep -o '__STATUS__[0-9]*' | grep -o '[0-9]*'
}

# 응답에서 본문 파싱
parse_body() {
    echo "$1" | sed 's/__STATUS__[0-9]*$//'
}

# JSON 필드 추출 (jq 없이도 동작하는 간단한 파서)
json_field() {
    local json="$1"
    local field="$2"
    echo "$json" | grep -o "\"${field}\":\"[^\"]*\"" | head -1 | sed "s/\"${field}\":\"//;s/\"//"
}

# ── 시나리오 1: 회원가입 ───────────────────────────────────────────────────────

echo "--- 시나리오 1: 회원가입 (POST /api/users) ---"

REGISTER_PAYLOAD=$(cat <<EOF
{
  "user": {
    "username": "${TEST_USERNAME}",
    "email": "${TEST_EMAIL}",
    "password": "${TEST_PASSWORD}"
  }
}
EOF
)

REGISTER_RESP=$(http_response \
    -X POST \
    -H "Content-Type: application/json" \
    -d "${REGISTER_PAYLOAD}" \
    "${API_URL}/users")

REGISTER_STATUS=$(parse_status "${REGISTER_RESP}")
REGISTER_BODY=$(parse_body "${REGISTER_RESP}")

if [ "${REGISTER_STATUS}" = "201" ]; then
    ok "회원가입 성공 (HTTP 201)"
else
    fail "회원가입 실패 (HTTP ${REGISTER_STATUS}, 기대값: 201)"
    echo "      응답: ${REGISTER_BODY}"
fi

# ── 시나리오 2: 로그인 및 JWT 토큰 추출 ──────────────────────────────────────

echo ""
echo "--- 시나리오 2: 로그인 (POST /api/users/login) ---"

LOGIN_PAYLOAD=$(cat <<EOF
{
  "user": {
    "email": "${TEST_EMAIL}",
    "password": "${TEST_PASSWORD}"
  }
}
EOF
)

LOGIN_RESP=$(http_response \
    -X POST \
    -H "Content-Type: application/json" \
    -d "${LOGIN_PAYLOAD}" \
    "${API_URL}/users/login")

LOGIN_STATUS=$(parse_status "${LOGIN_RESP}")
LOGIN_BODY=$(parse_body "${LOGIN_RESP}")

if [ "${LOGIN_STATUS}" = "200" ]; then
    ok "로그인 성공 (HTTP 200)"
else
    fail "로그인 실패 (HTTP ${LOGIN_STATUS}, 기대값: 200)"
    echo "      응답: ${LOGIN_BODY}"
fi

# JWT 토큰 추출
JWT_TOKEN=$(json_field "${LOGIN_BODY}" "token")

if [ -n "${JWT_TOKEN}" ]; then
    ok "JWT 토큰 추출 성공 (${JWT_TOKEN:0:20}...)"
else
    fail "JWT 토큰 추출 실패 — 응답에서 token 필드를 찾을 수 없음"
    echo "      응답 본문: ${LOGIN_BODY}"
    # 토큰 없이 다음 테스트 진행 불가
    echo ""
    echo "=== E2E 테스트 결과 ==="
    echo -e "  통과: ${GREEN}${PASS}${NC}  실패: ${RED}${FAIL}${NC}  건너뜀: ${YELLOW}${SKIP}${NC}"
    [ "${FAIL}" -gt 0 ] && exit 1 || exit 0
fi

# ── 시나리오 3: 현재 사용자 조회 ─────────────────────────────────────────────

echo ""
echo "--- 시나리오 3: 현재 사용자 조회 (GET /api/user) ---"

ME_RESP=$(http_response \
    -X GET \
    -H "Authorization: Token ${JWT_TOKEN}" \
    "${API_URL}/user")

ME_STATUS=$(parse_status "${ME_RESP}")
ME_BODY=$(parse_body "${ME_RESP}")

if [ "${ME_STATUS}" = "200" ]; then
    ok "현재 사용자 조회 성공 (HTTP 200)"

    # 응답에 이메일이 포함되어 있는지 확인
    ME_EMAIL=$(json_field "${ME_BODY}" "email")
    if [ "${ME_EMAIL}" = "${TEST_EMAIL}" ]; then
        ok "응답 이메일 일치 (${ME_EMAIL})"
    else
        fail "응답 이메일 불일치 (받은 값: '${ME_EMAIL}', 기대값: '${TEST_EMAIL}')"
    fi
else
    fail "현재 사용자 조회 실패 (HTTP ${ME_STATUS}, 기대값: 200)"
    echo "      응답: ${ME_BODY}"
fi

# 인증 없이 요청 시 401 확인
ME_UNAUTH_STATUS=$(http_status \
    -X GET \
    "${API_URL}/user")

if [ "${ME_UNAUTH_STATUS}" = "401" ]; then
    ok "인증 없이 /user 접근 시 401 반환"
else
    fail "인증 없이 /user 접근 시 기대값 401, 실제값 ${ME_UNAUTH_STATUS}"
fi

# ── 시나리오 4: 아티클 목록 조회 ─────────────────────────────────────────────

echo ""
echo "--- 시나리오 4: 아티클 목록 조회 (GET /api/articles) ---"

ARTICLES_RESP=$(http_response \
    -X GET \
    "${API_URL}/articles")

ARTICLES_STATUS=$(parse_status "${ARTICLES_RESP}")
ARTICLES_BODY=$(parse_body "${ARTICLES_RESP}")

if [ "${ARTICLES_STATUS}" = "200" ]; then
    ok "아티클 목록 조회 성공 (HTTP 200)"
else
    fail "아티클 목록 조회 실패 (HTTP ${ARTICLES_STATUS}, 기대값: 200)"
    echo "      응답: ${ARTICLES_BODY}"
fi

# limit 파라미터 동작 확인
ARTICLES_LIMIT_STATUS=$(http_status \
    -X GET \
    "${API_URL}/articles?limit=5&offset=0")

if [ "${ARTICLES_LIMIT_STATUS}" = "200" ]; then
    ok "limit/offset 파라미터 동작 확인 (HTTP 200)"
else
    fail "limit/offset 파라미터 테스트 실패 (HTTP ${ARTICLES_LIMIT_STATUS})"
fi

# ── 결과 출력 ─────────────────────────────────────────────────────────────────

echo ""
echo "=== E2E 테스트 결과 ==="
echo -e "  통과: ${GREEN}${PASS}${NC}  실패: ${RED}${FAIL}${NC}  건너뜀: ${YELLOW}${SKIP}${NC}"
echo ""

if [ "${FAIL}" -gt 0 ]; then
    echo -e "${RED}E2E 테스트 실패${NC}"
    exit 1
else
    echo -e "${GREEN}모든 E2E 테스트 통과${NC}"
    exit 0
fi
