# Makefile — drogon-realworld-example-app
#
# 주요 타겟:
#   make build   - CMake 빌드 (Release)
#   make test    - 단위 테스트 실행
#   make lint    - clang-tidy 린트
#   make format  - clang-format 적용
#   make check   - lint + test (pre-commit hook 용)
#   make e2e     - E2E 테스트 (플레이스홀더)
#   make clean   - 빌드 산출물 삭제

# ── 설정 ─────────────────────────────────────────────────────────────────────

BUILD_DIR    := build
BUILD_TYPE   := Release
CMAKE        := cmake
CTEST        := ctest
CLANG_TIDY   := clang-tidy
CLANG_FORMAT := clang-format
NPROC        := $(shell nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

# 린트/포맷 대상 소스 파일 (external/ 와 build/ 제외)
SRC_FILES := $(shell find . \
    -not -path './external/*' \
    -not -path './build/*'    \
    -not -path './.git/*'     \
    \( -name '*.cc' -o -name '*.h' -o -name '*.hpp' \) \
    2>/dev/null)

# compile_commands.json 경로 (clang-tidy 에 필요)
COMPILE_COMMANDS := $(BUILD_DIR)/compile_commands.json

.PHONY: all build test lint format check e2e clean help configure setup-hooks

# ── 기본 타겟 ─────────────────────────────────────────────────────────────────

all: build

help:
	@echo ""
	@echo "사용 가능한 타겟:"
	@echo "  make build        - CMake $(BUILD_TYPE) 빌드"
	@echo "  make test         - 단위 테스트 실행 (Google Test)"
	@echo "  make lint         - clang-tidy 정적 분석"
	@echo "  make format       - clang-format 자동 포맷 적용"
	@echo "  make check        - lint + test (pre-commit hook 용)"
	@echo "  make e2e          - E2E 테스트 (플레이스홀더)"
	@echo "  make setup-hooks  - Git hooks 설정 (.githooks/ 디렉토리 등록)"
	@echo "  make clean        - 빌드 산출물 삭제"
	@echo ""

# ── configure: CMake 설정 ────────────────────────────────────────────────────

configure: $(BUILD_DIR)/CMakeCache.txt

$(BUILD_DIR)/CMakeCache.txt:
	@echo "[configure] CMake 설정 중... (BUILD_TYPE=$(BUILD_TYPE))"
	$(CMAKE) -B $(BUILD_DIR) \
	    -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@echo "[configure] 완료"

# ── build: CMake 빌드 ────────────────────────────────────────────────────────

build: configure
	@echo "[build] 빌드 중... (jobs=$(NPROC))"
	$(CMAKE) --build $(BUILD_DIR) --config $(BUILD_TYPE) --parallel $(NPROC)
	@echo "[build] 완료"

# ── test: 단위 테스트 실행 ────────────────────────────────────────────────────

test: configure
	@echo "[test] CMake 테스트 빌드 중..."
	$(CMAKE) -B $(BUILD_DIR) \
	    -DCMAKE_BUILD_TYPE=Debug \
	    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	$(CMAKE) --build $(BUILD_DIR) --config Debug --parallel $(NPROC)
	@echo "[test] 단위 테스트 실행 중..."
	cd $(BUILD_DIR) && $(CTEST) --output-on-failure --parallel $(NPROC)
	@echo "[test] 완료"

# ── lint: clang-tidy 정적 분석 ───────────────────────────────────────────────

lint: $(COMPILE_COMMANDS)
	@echo "[lint] clang-tidy 분석 중..."
	@if [ -z "$(SRC_FILES)" ]; then \
	    echo "[lint] 분석할 소스 파일 없음"; \
	else \
	    $(CLANG_TIDY) \
	        -p $(BUILD_DIR) \
	        --config-file=.clang-tidy \
	        $(SRC_FILES) \
	    && echo "[lint] 완료 — 경고 없음" \
	    || (echo "[lint] 경고/에러 발견됨" && exit 1); \
	fi

$(COMPILE_COMMANDS): configure
	@# compile_commands.json 은 configure 단계에서 생성됨

# ── format: clang-format 자동 포맷 ──────────────────────────────────────────

format:
	@echo "[format] clang-format 적용 중..."
	@if [ -z "$(SRC_FILES)" ]; then \
	    echo "[format] 포맷할 소스 파일 없음"; \
	else \
	    $(CLANG_FORMAT) -i --style=file $(SRC_FILES) \
	    && echo "[format] 완료"; \
	fi

# 포맷 검사만 (변경 없음, CI 용)
format-check:
	@echo "[format-check] 포맷 검사 중..."
	@if [ -z "$(SRC_FILES)" ]; then \
	    echo "[format-check] 검사할 소스 파일 없음"; \
	else \
	    $(CLANG_FORMAT) --dry-run --Werror --style=file $(SRC_FILES) \
	    && echo "[format-check] 포맷 일치" \
	    || (echo "[format-check] 포맷 불일치 — make format 으로 수정하세요" && exit 1); \
	fi

# ── check: pre-commit hook 용 ────────────────────────────────────────────────
# lint 와 test 를 순서대로 실행. 하나라도 실패하면 중단.

check: lint test
	@echo "[check] 모든 검사 통과"

# ── e2e: E2E 테스트 플레이스홀더 ────────────────────────────────────────────

e2e:
	@echo "[e2e] E2E 테스트 — 아직 구현되지 않음 (플레이스홀더)"
	@echo "      향후 Newman(Postman), pytest-requests, 또는 k6 스크립트 추가 예정"
	@echo "      서버가 localhost:8080 에서 실행 중이어야 합니다"
	@exit 0

# ── setup-hooks: Git hooks 디렉토리 등록 ─────────────────────────────────────

setup-hooks:
	sh scripts/setup-hooks.sh

# ── clean: 빌드 산출물 삭제 ─────────────────────────────────────────────────

clean:
	@echo "[clean] 빌드 산출물 삭제 중..."
	$(CMAKE) --build $(BUILD_DIR) --target clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)
	@echo "[clean] 완료"
