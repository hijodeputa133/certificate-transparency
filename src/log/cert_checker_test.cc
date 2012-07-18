#include <gtest/gtest.h>
#include <openssl/ssl.h>
#include <string>

#include "../util/util.h"
#include "cert.h"
#include "cert_checker.h"

static const char kCertDir[] = "../test/testdata";

// Valid certificates.
// Self-signed
static const char kCaCert[] = "ca-cert.pem";
// Issued by ca-cert.pem
static const char kLeafCert[] = "test-cert.pem";
// Issued by ca-cert.pem
static const char kCaProtoCert[] = "ca-proto-cert.pem";
// Issued by ca-proto-cert.pem
static const char kProtoCert[] = "test-proto-cert.pem";
// Issued by ca-cert.pem
static const char kIntermediateCert[] = "intermediate-cert.pem";
// Issued by intermediate-cert.pem
static const char kChainLeafCert[] = "test2-cert.pem";

namespace {

class CertCheckerTest : public ::testing::Test {
 protected:
  std::string leaf_pem_;
  std::string ca_protocert_pem_;
  std::string protocert_pem_;
  std::string intermediate_pem_;
  std::string chain_leaf_pem_;
  CertChecker checker_;
  std::string cert_dir_;

  void SetUp() {
    cert_dir_ = std::string(kCertDir);
    ASSERT_TRUE(util::ReadTextFile(cert_dir_ + "/" + kLeafCert, &leaf_pem_));
    ASSERT_TRUE(util::ReadTextFile(cert_dir_ + "/" + kCaProtoCert,
                                   &ca_protocert_pem_));
    ASSERT_TRUE(util::ReadTextFile(cert_dir_ + "/" + kProtoCert,
                                   &protocert_pem_));
    ASSERT_TRUE(util::ReadTextFile(cert_dir_ + "/" + kIntermediateCert,
                                   &intermediate_pem_));
    ASSERT_TRUE(util::ReadTextFile(cert_dir_ + "/" + kChainLeafCert,
                                   &chain_leaf_pem_));
  }
};

TEST_F(CertCheckerTest, Certificate) {
  CertChain chain(leaf_pem_);
  ASSERT_TRUE(chain.IsLoaded());

  // Fail as we have no CA certs.
  EXPECT_FALSE(checker_.CheckCertChain(chain));

  // Load CA certs and expect success.
  EXPECT_TRUE(checker_.LoadTrustedCertificate(cert_dir_ + "/" + kCaCert));
  EXPECT_TRUE(checker_.CheckCertChain(chain));
}

TEST_F(CertCheckerTest, Intermediates) {
  // Load CA certs.
  EXPECT_TRUE(checker_.LoadTrustedCertificate(cert_dir_ + "/" + kCaCert));
  // A chain with an intermediate.
  CertChain chain(chain_leaf_pem_);
  ASSERT_TRUE(chain.IsLoaded());
  // Fail as it doesn't chain to a trusted CA.
  EXPECT_FALSE(checker_.CheckCertChain(chain));
  // Add the intermediate and expect success.
  chain.AddCert(new Cert(intermediate_pem_));
  EXPECT_TRUE(checker_.CheckCertChain(chain));

  // An invalid chain, with two certs in wrong order.
  CertChain invalid(intermediate_pem_ + chain_leaf_pem_);
  ASSERT_TRUE(invalid.IsLoaded());
  EXPECT_FALSE(checker_.CheckCertChain(invalid));
}

TEST_F(CertCheckerTest, ProtoCert) {
  const std::string chain_pem = protocert_pem_ + ca_protocert_pem_;
  ProtoCertChain chain(chain_pem);

  ASSERT_TRUE(chain.IsLoaded());
  EXPECT_TRUE(chain.IsWellFormed());

  // Fail as we have no CA certs.
  EXPECT_FALSE(checker_.CheckProtoCertChain(chain));

  // Load CA certs and expect success.
  checker_.LoadTrustedCertificate(cert_dir_ + "/" + kCaCert);
  EXPECT_TRUE(checker_.CheckProtoCertChain(chain));

  // A second, invalid chain, with no CA protocert.
  ProtoCertChain chain2(protocert_pem_);
  ASSERT_TRUE(chain2.IsLoaded());
  EXPECT_FALSE(chain2.IsWellFormed());
  EXPECT_FALSE(checker_.CheckProtoCertChain(chain2));
}

}  // namespace

int main(int argc, char**argv) {
  ::testing::InitGoogleTest(&argc, argv);
  SSL_library_init();
  return RUN_ALL_TESTS();
}