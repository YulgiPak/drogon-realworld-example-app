// tests/test_bcrypt.cc
//
// BCrypt 유틸리티 단위 테스트
// 실제 코드에서 사용하는 BCrypt::generateHash / BCrypt::validatePassword 검증.
//
// 테스트 대상 (controllers/api_UsersController.cc):
//   auto encryptedPass = BCrypt::generateHash(newUser.getValueOfPassword());
//   if (BCrypt::validatePassword(pNewUser.getValueOfPassword(), user.getValueOfPassword()))

#include <gtest/gtest.h>
#include <bcrypt/BCrypt.hpp>
#include <string>

class BcryptTest : public ::testing::Test
{
protected:
    const std::string SAMPLE_PASSWORD      = "SecureP@ssw0rd!";
    const std::string ANOTHER_PASSWORD     = "AnotherP@ss123";
    const std::string EMPTY_PASSWORD       = "";
    const std::string LONG_PASSWORD        = std::string(72, 'a'); // bcrypt 최대 처리 길이
};

// 해시 생성: 빈 문자열이 아니어야 한다
TEST_F(BcryptTest, GenerateHash_NotEmpty)
{
    std::string hash = BCrypt::generateHash(SAMPLE_PASSWORD);
    EXPECT_FALSE(hash.empty());
}

// 해시 생성: bcrypt 해시 포맷 확인 ("$2" 로 시작)
TEST_F(BcryptTest, GenerateHash_HasBcryptPrefix)
{
    std::string hash = BCrypt::generateHash(SAMPLE_PASSWORD);
    EXPECT_EQ(hash.substr(0, 2), "$2");
}

// 해시 생성: 동일 비밀번호라도 매번 다른 해시 (salt 기반)
TEST_F(BcryptTest, GenerateHash_SamePasswordDifferentHash)
{
    std::string hash1 = BCrypt::generateHash(SAMPLE_PASSWORD);
    std::string hash2 = BCrypt::generateHash(SAMPLE_PASSWORD);
    EXPECT_NE(hash1, hash2);
}

// 검증: 올바른 비밀번호 → true
TEST_F(BcryptTest, ValidatePassword_CorrectPassword_ReturnsTrue)
{
    std::string hash = BCrypt::generateHash(SAMPLE_PASSWORD);
    EXPECT_TRUE(BCrypt::validatePassword(SAMPLE_PASSWORD, hash));
}

// 검증: 잘못된 비밀번호 → false
TEST_F(BcryptTest, ValidatePassword_WrongPassword_ReturnsFalse)
{
    std::string hash = BCrypt::generateHash(SAMPLE_PASSWORD);
    EXPECT_FALSE(BCrypt::validatePassword(ANOTHER_PASSWORD, hash));
}

// 검증: 비밀번호 대소문자 구분
TEST_F(BcryptTest, ValidatePassword_CaseSensitive)
{
    std::string hash = BCrypt::generateHash(SAMPLE_PASSWORD);
    std::string lowerPassword = "securep@ssw0rd!"; // 소문자 변환
    EXPECT_FALSE(BCrypt::validatePassword(lowerPassword, hash));
}

// 검증: 빈 비밀번호 해싱 후 검증
TEST_F(BcryptTest, ValidatePassword_EmptyPassword_CorrectHash_ReturnsTrue)
{
    std::string hash = BCrypt::generateHash(EMPTY_PASSWORD);
    EXPECT_TRUE(BCrypt::validatePassword(EMPTY_PASSWORD, hash));
}

// 검증: 빈 비밀번호 해시에 다른 비밀번호 → false
TEST_F(BcryptTest, ValidatePassword_EmptyPasswordHash_WrongInput_ReturnsFalse)
{
    std::string hash = BCrypt::generateHash(EMPTY_PASSWORD);
    EXPECT_FALSE(BCrypt::validatePassword(SAMPLE_PASSWORD, hash));
}

// 검증: 긴 비밀번호 (72바이트 경계)
TEST_F(BcryptTest, ValidatePassword_LongPassword_ReturnsTrue)
{
    std::string hash = BCrypt::generateHash(LONG_PASSWORD);
    EXPECT_TRUE(BCrypt::validatePassword(LONG_PASSWORD, hash));
}

// 검증: 특수문자 포함 비밀번호
TEST_F(BcryptTest, ValidatePassword_SpecialChars_ReturnsTrue)
{
    std::string specialPass = "P@$$w0rd!#&*()[]{}";
    std::string hash = BCrypt::generateHash(specialPass);
    EXPECT_TRUE(BCrypt::validatePassword(specialPass, hash));
}

// 검증: 공백 포함 비밀번호
TEST_F(BcryptTest, ValidatePassword_WithSpaces_ReturnsTrue)
{
    std::string spacePass = "pass word with spaces";
    std::string hash = BCrypt::generateHash(spacePass);
    EXPECT_TRUE(BCrypt::validatePassword(spacePass, hash));
}

// 검증: 해시 자체를 비밀번호로 입력 → false (해시 재해시 방지)
TEST_F(BcryptTest, ValidatePassword_HashAsPassword_ReturnsFalse)
{
    std::string hash = BCrypt::generateHash(SAMPLE_PASSWORD);
    EXPECT_FALSE(BCrypt::validatePassword(hash, hash));
}

// login 시나리오: newUser 등록 → login 검증 흐름 통합
TEST_F(BcryptTest, LoginScenario_RegisterThenLogin_Success)
{
    // newUser: 비밀번호 해싱 후 저장
    const std::string rawPassword = "MyLoginPass!23";
    std::string storedHash = BCrypt::generateHash(rawPassword);

    // login: 입력 비밀번호와 저장된 해시 비교
    EXPECT_TRUE(BCrypt::validatePassword(rawPassword, storedHash));
    EXPECT_FALSE(BCrypt::validatePassword("WrongPass!23", storedHash));
}
