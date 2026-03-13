// tests/test_input_validation.cc
//
// 입력 유효성 검증 단위 테스트
// Drogon / DB 의존성 없이 순수 비즈니스 규칙 로직만 검증합니다.
//
// 테스트 대상 (controllers/api_UsersController.cc - checkInputUser):
//   if (user.getEmail() == nullptr || user.getPassword() == nullptr || user.getUsername() == nullptr)
//
// DB 중복 체크(findBy)는 비동기 콜백 기반이라 단위 테스트에서 제외.
// 순수 null 체크 및 비즈니스 규칙 검증 로직만 추출하여 테스트.
//
// 테스트 대상 (utils/jwtServive.cc - getCurrentUserIdFromRequest):
//   auto token = req->getHeader("Authorization").substr(7);
//   — "Token " prefix 7자 제거 후 파싱

#include <gtest/gtest.h>
#include <string>
#include <optional>
#include <functional>

// ---- 테스트용 UserInput 구조체 ------------------------------------------
// Drogon ORM Users 모델 의존성 없이 동일한 null 체크 로직을 재현.
// 실제 Drogon ORM은 shared_ptr<string> 기반 nullable 필드를 사용하므로
// nullptr 비교 동작을 std::optional 로 모사.

struct UserInput
{
    std::optional<std::string> email;
    std::optional<std::string> password;
    std::optional<std::string> username;
    std::optional<std::string> bio;
    std::optional<std::string> image;
};

// checkInputUser 의 null 체크 부분 — 순수 함수로 추출
// 반환값: true = 필드 모두 존재, false = 필수 필드 누락
inline bool hasRequiredFields(const UserInput& user)
{
    return user.email.has_value()
        && user.password.has_value()
        && user.username.has_value();
}

// ---- 이메일 형식 검증 (비즈니스 규칙) ------------------------------------
// 실제 코드에는 없지만 워크숍에서 추가 권장되는 검증 로직 예시
inline bool isValidEmail(const std::string& email)
{
    // 단순 검증: @ 와 . 이 포함되어야 하고, @ 뒤에 . 이 있어야 함
    auto atPos  = email.find('@');
    if (atPos == std::string::npos || atPos == 0) return false;
    auto dotPos = email.find('.', atPos);
    if (dotPos == std::string::npos || dotPos == email.size() - 1) return false;
    return true;
}

// ---- Authorization 헤더 파싱 로직 ---------------------------------------
// "Token " prefix (7자) 추출 로직
inline std::optional<std::string> extractTokenFromHeader(const std::string& authHeader)
{
    const std::string PREFIX = "Token ";
    if (authHeader.size() <= PREFIX.size()) {
        return std::nullopt;
    }
    if (authHeader.substr(0, PREFIX.size()) != PREFIX) {
        return std::nullopt;
    }
    return authHeader.substr(PREFIX.size());
}

// ---- 테스트 케이스 -------------------------------------------------------

class InputValidationTest : public ::testing::Test
{
protected:
    UserInput makeValidUser()
    {
        return {
            .email    = "user@example.com",
            .password = "SecurePass!1",
            .username = "testuser",
        };
    }
};

// --- 필수 필드 null 체크 ---

// 모든 필수 필드 존재 → true
TEST_F(InputValidationTest, HasRequiredFields_AllPresent_ReturnsTrue)
{
    auto user = makeValidUser();
    EXPECT_TRUE(hasRequiredFields(user));
}

// email 누락 → false
TEST_F(InputValidationTest, HasRequiredFields_MissingEmail_ReturnsFalse)
{
    auto user = makeValidUser();
    user.email = std::nullopt;
    EXPECT_FALSE(hasRequiredFields(user));
}

// password 누락 → false
TEST_F(InputValidationTest, HasRequiredFields_MissingPassword_ReturnsFalse)
{
    auto user = makeValidUser();
    user.password = std::nullopt;
    EXPECT_FALSE(hasRequiredFields(user));
}

// username 누락 → false
TEST_F(InputValidationTest, HasRequiredFields_MissingUsername_ReturnsFalse)
{
    auto user = makeValidUser();
    user.username = std::nullopt;
    EXPECT_FALSE(hasRequiredFields(user));
}

// 세 필드 모두 누락 → false
TEST_F(InputValidationTest, HasRequiredFields_AllMissing_ReturnsFalse)
{
    UserInput empty{};
    EXPECT_FALSE(hasRequiredFields(empty));
}

// bio, image 는 선택적 필드 — 없어도 유효
TEST_F(InputValidationTest, HasRequiredFields_OptionalFieldsMissing_StillValid)
{
    auto user = makeValidUser();
    user.bio   = std::nullopt;
    user.image = std::nullopt;
    EXPECT_TRUE(hasRequiredFields(user));
}

// --- 이메일 형식 검증 ---

TEST_F(InputValidationTest, IsValidEmail_NormalEmail_ReturnsTrue)
{
    EXPECT_TRUE(isValidEmail("user@example.com"));
}

TEST_F(InputValidationTest, IsValidEmail_SubdomainEmail_ReturnsTrue)
{
    EXPECT_TRUE(isValidEmail("user@mail.example.co.kr"));
}

TEST_F(InputValidationTest, IsValidEmail_NoAtSign_ReturnsFalse)
{
    EXPECT_FALSE(isValidEmail("userexample.com"));
}

TEST_F(InputValidationTest, IsValidEmail_AtSignAtStart_ReturnsFalse)
{
    EXPECT_FALSE(isValidEmail("@example.com"));
}

TEST_F(InputValidationTest, IsValidEmail_NoDotAfterAt_ReturnsFalse)
{
    EXPECT_FALSE(isValidEmail("user@examplecom"));
}

TEST_F(InputValidationTest, IsValidEmail_DotAtEnd_ReturnsFalse)
{
    EXPECT_FALSE(isValidEmail("user@example."));
}

TEST_F(InputValidationTest, IsValidEmail_EmptyString_ReturnsFalse)
{
    EXPECT_FALSE(isValidEmail(""));
}

// --- Authorization 헤더 파싱 ---

// 올바른 "Token <value>" 형식 → 토큰 값 추출
TEST_F(InputValidationTest, ExtractToken_ValidPrefix_ReturnsToken)
{
    auto result = extractTokenFromHeader("Token abc123def456");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "abc123def456");
}

// "Token " 만 있고 실제 토큰 없음 → nullopt
TEST_F(InputValidationTest, ExtractToken_PrefixOnly_ReturnsNullopt)
{
    auto result = extractTokenFromHeader("Token ");
    // 길이가 정확히 PREFIX.size() 이므로 nullopt
    EXPECT_FALSE(result.has_value());
}

// 빈 문자열 → nullopt
TEST_F(InputValidationTest, ExtractToken_EmptyHeader_ReturnsNullopt)
{
    auto result = extractTokenFromHeader("");
    EXPECT_FALSE(result.has_value());
}

// "Bearer " 형식 → nullopt (잘못된 prefix)
TEST_F(InputValidationTest, ExtractToken_BearerPrefix_ReturnsNullopt)
{
    auto result = extractTokenFromHeader("Bearer some.token.value");
    EXPECT_FALSE(result.has_value());
}

// 소문자 "token " → nullopt (대소문자 구분)
TEST_F(InputValidationTest, ExtractToken_LowercasePrefix_ReturnsNullopt)
{
    auto result = extractTokenFromHeader("token abc123");
    EXPECT_FALSE(result.has_value());
}

// 긴 JWT 토큰 (실제 jwt 형식)
TEST_F(InputValidationTest, ExtractToken_RealJwtFormat_ReturnsToken)
{
    std::string fakeJwt = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXUyJ9"
                          ".eyJ1c2VyIjoiNDIiLCJpc3MiOiJhdXRoMCJ9"
                          ".HMAC_SIGNATURE";
    auto result = extractTokenFromHeader("Token " + fakeJwt);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), fakeJwt);
}

// --- PUT /user update() 버그 재현 테스트 ---------------------------------
// 버그: username/password/bio/image 필드를 모두 setEmail()로 호출하는 오타
// 아래 테스트는 버그가 수정되었을 때의 올바른 동작을 명세합니다.

struct UserUpdateFields
{
    std::optional<std::string> email;
    std::optional<std::string> username;
    std::optional<std::string> password;
    std::optional<std::string> bio;
    std::optional<std::string> image;
};

// 업데이트 적용 로직: 버그 수정 후 올바른 필드별 setter 동작 명세
struct UpdatedUser
{
    std::string email    = "original@example.com";
    std::string username = "original_user";
    std::string password = "original_hash";
    std::string bio      = "original bio";
    std::string image    = "original.png";

    void applyUpdate(const UserUpdateFields& patch)
    {
        if (patch.email.has_value())    email    = patch.email.value();
        if (patch.username.has_value()) username = patch.username.value();
        if (patch.password.has_value()) password = patch.password.value();
        if (patch.bio.has_value())      bio      = patch.bio.value();
        if (patch.image.has_value())    image    = patch.image.value();
    }
};

// email 만 업데이트 → email 만 변경
TEST_F(InputValidationTest, UpdateUser_OnlyEmail_UpdatesEmailOnly)
{
    UpdatedUser user;
    UserUpdateFields patch{ .email = "new@example.com" };
    user.applyUpdate(patch);

    EXPECT_EQ(user.email,    "new@example.com");
    EXPECT_EQ(user.username, "original_user");   // 변경 없음
    EXPECT_EQ(user.password, "original_hash");   // 변경 없음
}

// username 만 업데이트 → username 만 변경 (버그 수정 후 기대 동작)
TEST_F(InputValidationTest, UpdateUser_OnlyUsername_UpdatesUsernameOnly)
{
    UpdatedUser user;
    UserUpdateFields patch{ .username = "new_username" };
    user.applyUpdate(patch);

    EXPECT_EQ(user.username, "new_username");
    EXPECT_EQ(user.email,    "original@example.com"); // 변경 없음
}

// 모든 필드 업데이트
TEST_F(InputValidationTest, UpdateUser_AllFields_UpdatesAll)
{
    UpdatedUser user;
    UserUpdateFields patch{
        .email    = "new@example.com",
        .username = "newuser",
        .password = "new_hash",
        .bio      = "new bio",
        .image    = "new.png",
    };
    user.applyUpdate(patch);

    EXPECT_EQ(user.email,    "new@example.com");
    EXPECT_EQ(user.username, "newuser");
    EXPECT_EQ(user.password, "new_hash");
    EXPECT_EQ(user.bio,      "new bio");
    EXPECT_EQ(user.image,    "new.png");
}

// nullopt 패치 → 아무것도 변경 안 됨
TEST_F(InputValidationTest, UpdateUser_EmptyPatch_ChangesNothing)
{
    UpdatedUser user;
    UserUpdateFields emptyPatch{};
    user.applyUpdate(emptyPatch);

    EXPECT_EQ(user.email,    "original@example.com");
    EXPECT_EQ(user.username, "original_user");
    EXPECT_EQ(user.password, "original_hash");
    EXPECT_EQ(user.bio,      "original bio");
    EXPECT_EQ(user.image,    "original.png");
}
