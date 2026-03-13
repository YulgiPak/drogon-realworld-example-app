// tests/test_jwtService.cc
//
// jwtService 단위 테스트
// Drogon app() 의존성을 피하기 위해 jwt-cpp를 직접 사용하여
// 동일한 로직을 검증합니다.
//
// 테스트 대상 로직:
//   - JWT 토큰 생성 (HS256, issuer "auth0", "user" payload claim)
//   - JWT 파싱 및 user_id 추출
//   - 잘못된 토큰 처리 (optional nullopt 반환)
//   - Authorization 헤더에서 "Token " prefix 제거 (7자 substr)

#include <gtest/gtest.h>
#include <jwt-cpp/jwt.h>
#include <optional>
#include <string>
#include <stdexcept>

// ---- 테스트용 jwtService 로직 (Drogon app() 의존성 제거) ---------------
// 실제 jwtServive.cc 의 로직을 인라인으로 재현하여 순수 단위 테스트 수행.
// secret / duration 은 테스트 고정값 사용.

namespace test_jwt {

static const std::string TEST_SECRET = "test-secret-key";
static const std::string ISSUER     = "auth0";

// generateFromUser() 동등 로직: user_id 를 직접 받아 토큰 생성
inline std::string generateTokenForUserId(int userId)
{
    return jwt::create()
        .set_issuer(ISSUER)
        .set_type("JWS")
        .set_payload_claim("user", jwt::claim(std::to_string(userId)))
        .sign(jwt::algorithm::hs256{TEST_SECRET});
}

// getUserIdFromJwt() 동등 로직
inline std::optional<int> getUserIdFromToken(const std::string& token)
{
    try {
        auto decoded  = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{TEST_SECRET})
            .with_issuer(ISSUER);
        verifier.verify(decoded);
        return std::stoi(decoded.get_payload_claim("user").as_string());
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// getCurrentUserIdFromRequest() 헤더 파싱 로직:
//   Authorization 헤더값에서 첫 7자("Token ") 를 잘라낸 뒤 파싱
inline std::optional<int> getUserIdFromAuthHeader(const std::string& authHeader)
{
    // "Token " 접두어가 없으면 유효하지 않은 헤더
    if (authHeader.size() <= 7) {
        return std::nullopt;
    }
    auto token = authHeader.substr(7);
    return getUserIdFromToken(token);
}

} // namespace test_jwt

// ---- 테스트 케이스 -------------------------------------------------------

class JwtServiceTest : public ::testing::Test
{
protected:
    const int    SAMPLE_USER_ID = 42;
    std::string  validToken;

    void SetUp() override
    {
        validToken = test_jwt::generateTokenForUserId(SAMPLE_USER_ID);
    }
};

// 토큰 생성: 빈 문자열이 아니어야 한다
TEST_F(JwtServiceTest, GenerateToken_NotEmpty)
{
    EXPECT_FALSE(validToken.empty());
}

// 토큰 생성: JWT 형식 (헤더.페이로드.서명 — 점 2개)
TEST_F(JwtServiceTest, GenerateToken_HasThreeParts)
{
    size_t firstDot  = validToken.find('.');
    size_t secondDot = validToken.find('.', firstDot + 1);
    ASSERT_NE(firstDot,  std::string::npos);
    ASSERT_NE(secondDot, std::string::npos);
    EXPECT_EQ(std::count(validToken.begin(), validToken.end(), '.'), 2);
}

// 파싱: 유효한 토큰에서 user_id 가 정확히 추출되어야 한다
TEST_F(JwtServiceTest, ParseToken_ValidToken_ReturnsCorrectUserId)
{
    auto result = test_jwt::getUserIdFromToken(validToken);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), SAMPLE_USER_ID);
}

// 파싱: user_id 0 (경계값)
TEST_F(JwtServiceTest, ParseToken_UserIdZero_ReturnsZero)
{
    auto token  = test_jwt::generateTokenForUserId(0);
    auto result = test_jwt::getUserIdFromToken(token);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0);
}

// 파싱: user_id 음수 — 생성 후 파싱 왕복 일관성
TEST_F(JwtServiceTest, ParseToken_NegativeUserId_RoundTrip)
{
    auto token  = test_jwt::generateTokenForUserId(-1);
    auto result = test_jwt::getUserIdFromToken(token);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), -1);
}

// 잘못된 토큰: 완전히 잘못된 문자열 → nullopt
TEST_F(JwtServiceTest, ParseToken_InvalidString_ReturnsNullopt)
{
    auto result = test_jwt::getUserIdFromToken("this.is.not.a.valid.jwt");
    EXPECT_FALSE(result.has_value());
}

// 잘못된 토큰: 빈 문자열 → nullopt
TEST_F(JwtServiceTest, ParseToken_EmptyString_ReturnsNullopt)
{
    auto result = test_jwt::getUserIdFromToken("");
    EXPECT_FALSE(result.has_value());
}

// 잘못된 토큰: 다른 secret 으로 서명된 토큰 → nullopt (서명 불일치)
TEST_F(JwtServiceTest, ParseToken_WrongSecret_ReturnsNullopt)
{
    // 다른 secret 으로 토큰 생성
    std::string foreignToken = jwt::create()
        .set_issuer("auth0")
        .set_type("JWS")
        .set_payload_claim("user", jwt::claim(std::string("42")))
        .sign(jwt::algorithm::hs256{"wrong-secret"});

    auto result = test_jwt::getUserIdFromToken(foreignToken);
    EXPECT_FALSE(result.has_value());
}

// 잘못된 토큰: issuer 불일치 → nullopt
TEST_F(JwtServiceTest, ParseToken_WrongIssuer_ReturnsNullopt)
{
    std::string wrongIssuerToken = jwt::create()
        .set_issuer("not-auth0")
        .set_type("JWS")
        .set_payload_claim("user", jwt::claim(std::string("42")))
        .sign(jwt::algorithm::hs256{test_jwt::TEST_SECRET});

    auto result = test_jwt::getUserIdFromToken(wrongIssuerToken);
    EXPECT_FALSE(result.has_value());
}

// Authorization 헤더: 올바른 "Token <jwt>" 형식
TEST_F(JwtServiceTest, AuthHeader_ValidTokenPrefix_ReturnsUserId)
{
    std::string authHeader = "Token " + validToken;
    auto result = test_jwt::getUserIdFromAuthHeader(authHeader);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), SAMPLE_USER_ID);
}

// Authorization 헤더: "Bearer " 형식 (잘못된 형식) → nullopt 또는 파싱 실패
// 실제 코드는 substr(7) 만 하므로 Bearer 토큰도 7자 이후를 파싱 시도함
// — 서명이 다르므로 결국 nullopt
TEST_F(JwtServiceTest, AuthHeader_BearerPrefix_ReturnsNullopt)
{
    // Bearer + 유효한 JWT 이지만 secret 이 다르면 실패
    std::string wrongToken = jwt::create()
        .set_issuer("auth0")
        .set_payload_claim("user", jwt::claim(std::string("42")))
        .sign(jwt::algorithm::hs256{"bearer-secret"});
    std::string authHeader = "Bearer " + wrongToken;
    auto result = test_jwt::getUserIdFromAuthHeader(authHeader);
    EXPECT_FALSE(result.has_value());
}

// Authorization 헤더: 너무 짧은 헤더 (7자 이하) → nullopt
TEST_F(JwtServiceTest, AuthHeader_TooShort_ReturnsNullopt)
{
    auto result = test_jwt::getUserIdFromAuthHeader("Token");
    EXPECT_FALSE(result.has_value());
}

// Authorization 헤더: 빈 문자열 → nullopt
TEST_F(JwtServiceTest, AuthHeader_Empty_ReturnsNullopt)
{
    auto result = test_jwt::getUserIdFromAuthHeader("");
    EXPECT_FALSE(result.has_value());
}

// 동일 user_id 에 대해 매번 다른 토큰이 생성될 수 있음 (타임스탬프 미사용 시 동일)
// — 현재 구현에서는 issued_at/expires_at 주석 처리됨 → 동일 토큰 생성
TEST_F(JwtServiceTest, GenerateToken_SameUserIdProducesSameToken)
{
    auto token1 = test_jwt::generateTokenForUserId(SAMPLE_USER_ID);
    auto token2 = test_jwt::generateTokenForUserId(SAMPLE_USER_ID);
    // 타임스탬프 없으므로 결정론적으로 동일해야 한다
    EXPECT_EQ(token1, token2);
}
