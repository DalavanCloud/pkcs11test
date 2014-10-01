// Copyright 2013-2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// PKCS#11 s11.10: Message digesting functions
//   C_DigestInit
//   C_Digest
//   C_DigestUpdate
//   C_DigestKey
//   C_DigestFinal
#include <string>
#include <cstdlib>
#include "pkcs11test.h"

#include <map>
#include <sstream>
#include <string>

using namespace std;  // So sue me

namespace pkcs11 {
namespace test {

map<CK_MECHANISM_TYPE, int> kDigestSize = {
  {CKM_MD5, 16},
  {CKM_SHA_1, 20},
  {CKM_SHA256, 256/8},
  {CKM_SHA384, 384/8},
  {CKM_SHA512, 512/8}
};

map<CK_MECHANISM_TYPE, map<string, string>> kTestVectors = {
  {CKM_MD5, {  // RFC 1321 A.5
      {"", "d41d8cd98f00b204e9800998ecf8427e"},
      {"a", "0cc175b9c0f1b6a831c399e269772661"},
      {"abc", "900150983cd24fb0d6963f7d28e17f72"},
      {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
      {"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
      {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
       "d174ab98d277d9f5a5611c2c9f419d9f"},
      {"12345678901234567890123456789012345678901234567890123456789012345678901234567890",
       "57edf4a22be3c955ac49da2e2107b67a"},
    }},
  {CKM_SHA_1, {  // http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA_All.pdf
      {"", "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
      {"abc", "a9993e364706816aba3e25717850c26c9cd0d89d"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "84983e441c3bd26ebaae4aa1f95129e5e54670f1"},
    }},
  {CKM_SHA256, {
      {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
      {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
    }},
  {CKM_SHA384, {
      {"", "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"},
      {"abc", "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "3391fdddfc8dc7393707a65b1b4709397cf8b1d162af05abfe8f450de5f36bc6b0455a8520bc4e6f5fe95b1fe3c8452b"},
    }},
  {CKM_SHA512, {
      {"", "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"},
      {"abc", "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445"},
    }},
};

TEST_F(ReadOnlySessionTest, DigestInitInvalid) {
  CK_MECHANISM mechanism = {999, NULL_PTR, 0};
  EXPECT_CKR(CKR_MECHANISM_INVALID,
             g_fns->C_DigestInit(session_, &mechanism));
}

class DigestTest : public ReadOnlySessionTest,
                   public ::testing::WithParamInterface<CK_MECHANISM_TYPE> {
 public:
  DigestTest()
    : mechanism_({GetParam(), NULL_PTR, 0}),
      digestsize_(kDigestSize[GetParam()]),
      datalen_(std::rand() % 1024),
      data_(randmalloc(datalen_)) {
    if (g_verbose) cout << "DATA:  " << hex_data(data_.get(), min(40, datalen_))
                        << ((datalen_>40) ? "..." : "") << endl;
  }

  string PerformDigest(CK_BYTE_PTR data, CK_ULONG datalen) {
    CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
    if (rv == CKR_MECHANISM_INVALID) return "unimplemented";
    EXPECT_CKR_OK(rv);
    CK_BYTE buffer[512];
    CK_ULONG digest_len = sizeof(buffer);
    EXPECT_CKR_OK(g_fns->C_Digest(session_, data, datalen, buffer, &digest_len));
    EXPECT_EQ(digestsize_, digest_len);
    if (g_verbose) cout << "DIGEST: " << hex_data(buffer, digest_len) << endl;
    return string(reinterpret_cast<char*>(buffer), digest_len);
  }
  string PerformDigest() {
    return PerformDigest(data_.get(), datalen_);
  }
  string PerformDigest(const string& data) {
    return PerformDigest((CK_BYTE_PTR)data.data(), data.length());
  }

  string PerformDigestUpdate(CK_BYTE_PTR data, CK_ULONG datalen) {
    CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
    if (rv == CKR_MECHANISM_INVALID) return "unimplemented";
    EXPECT_CKR_OK(rv);
    const int kChunkSize = 10;
    CK_BYTE_PTR p = data;
    int dataleft = datalen;
    int count = 0;
    while (dataleft > 0) {
      int size = min(kChunkSize, dataleft);
      EXPECT_CKR_OK(g_fns->C_DigestUpdate(session_, p, size));
      p += size;
      dataleft -= size;
      ++count;
    }

    CK_BYTE buffer[512];
    CK_ULONG digest_len = sizeof(buffer);
    EXPECT_CKR_OK(g_fns->C_DigestFinal(session_, buffer, &digest_len));
    EXPECT_EQ(digestsize_, digest_len);
    if (g_verbose) cout << "DIGEST: " << hex_data(buffer, digest_len) << endl;
    return string(reinterpret_cast<char*>(buffer), digest_len);
  }
  string PerformDigestUpdate() {
    return PerformDigestUpdate(data_.get(), datalen_);
  }

 protected:
  CK_MECHANISM mechanism_;
  const int digestsize_;
  const int datalen_;
  unique_ptr<CK_BYTE, freer> data_;
};

#define SKIP_IF_UNIMPLEMENTED(d) \
    if ((d) == "unimplemented") { \
      stringstream ss; \
      ss << "Digest type " << mechanism_type_name(mechanism_.mechanism) << " not implemented"; \
      TEST_SKIPPED(ss.str()); \
      return; \
    }
#define SKIP_IF_UNIMPLEMENTED_RV(rv) \
    if ((rv) == CKR_MECHANISM_INVALID) {  \
      stringstream ss; \
      ss << "Digest type " << mechanism_type_name(mechanism_.mechanism) << " not implemented"; \
      TEST_SKIPPED(ss.str()); \
      return; \
    }

TEST_P(DigestTest, CompareIncremental) {
  string d1 = PerformDigest();
  string d2 = PerformDigestUpdate();
  SKIP_IF_UNIMPLEMENTED(d1);
  EXPECT_EQ(hex_data(d1), hex_data(d2));
}

TEST_P(DigestTest, DigestKey) {
  CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
  SKIP_IF_UNIMPLEMENTED_RV(rv);
  EXPECT_CKR_OK(rv);

  vector<CK_ATTRIBUTE_TYPE> attrs = {CKA_ENCRYPT, CKA_DECRYPT};
  SecretKey key(session_, attrs, CKM_DES_KEY_GEN);

  rv = g_fns->C_DigestKey(session_, key.handle());
  if (rv == CKR_KEY_INDIGESTIBLE) {
    stringstream ss;
    ss << mechanism_type_name(mechanism_.mechanism) << " cannot digest DES key";
    TEST_SKIPPED(ss.str());
    return;
  }
  EXPECT_CKR_OK(rv);

  CK_BYTE buffer[512];
  CK_ULONG digest_len = sizeof(buffer);
  EXPECT_CKR_OK(g_fns->C_DigestFinal(session_, buffer, &digest_len));
}

TEST_P(DigestTest, DigestKeyInvalid) {
  CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
  SKIP_IF_UNIMPLEMENTED_RV(rv);
  EXPECT_CKR_OK(rv);

  vector<CK_ATTRIBUTE_TYPE> attrs = {CKA_ENCRYPT, CKA_DECRYPT};
  SecretKey key(session_, attrs, CKM_DES_KEY_GEN);

  rv = g_fns->C_DigestKey(session_, key.handle());
  if (rv == CKR_KEY_INDIGESTIBLE) {
    stringstream ss;
    ss << mechanism_type_name(mechanism_.mechanism) << " cannot digest DES key";
    TEST_SKIPPED(ss.str());
    return;
  }
  EXPECT_CKR_OK(rv);
  EXPECT_CKR(CKR_SESSION_HANDLE_INVALID,
             g_fns->C_DigestKey(INVALID_SESSION_HANDLE, key.handle()));
  EXPECT_CKR(CKR_KEY_HANDLE_INVALID,
             g_fns->C_DigestKey(session_, INVALID_OBJECT_HANDLE));

  // Spec is not definitive as to whether an error return code terminates
  // the active digest operation (as it would for an error in DigestUpdate).
  CK_BYTE buffer[512];
  CK_ULONG digest_len = sizeof(buffer);
  g_fns->C_DigestFinal(session_, buffer, &digest_len);
}

TEST_P(DigestTest, DigestInvalidSession) {
  EXPECT_CKR(CKR_SESSION_HANDLE_INVALID, g_fns->C_DigestInit(INVALID_SESSION_HANDLE, &mechanism_));
}

TEST_P(DigestTest, DigestGetSpace) {
  CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
  SKIP_IF_UNIMPLEMENTED_RV(rv);
  EXPECT_CKR_OK(rv);

  CK_BYTE buffer[512];
  CK_ULONG digest_len = 0;
  // Provide no buffer => get OK return code and the required length.
  EXPECT_CKR_OK(g_fns->C_Digest(session_, data_.get(), datalen_, NULL_PTR, &digest_len));
  EXPECT_EQ(digestsize_, digest_len);

  // Provide too-small buffer => get too-small return code and the required length.
  memset(buffer, 0xAB, sizeof(buffer));
  digest_len = 2;
  EXPECT_CKR(CKR_BUFFER_TOO_SMALL,
             g_fns->C_Digest(session_, data_.get(), datalen_, buffer, &digest_len));
  EXPECT_EQ(digestsize_, digest_len);
  // Buffer unaffected.
  EXPECT_EQ(0xAB, buffer[3]);
  EXPECT_EQ(0xAB, buffer[0]);

  EXPECT_CKR_OK(g_fns->C_Digest(session_, data_.get(), datalen_, buffer, &digest_len));
}

TEST_P(DigestTest, DigestUpdateGetSpace) {
  CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
  SKIP_IF_UNIMPLEMENTED_RV(rv);
  EXPECT_CKR_OK(rv);

  EXPECT_CKR_OK(g_fns->C_DigestUpdate(session_, data_.get(), datalen_));

  CK_BYTE buffer[512];
  CK_ULONG digest_len = 0;
  // Provide no buffer => get OK return code and the required length.
  EXPECT_CKR_OK(g_fns->C_DigestFinal(session_, NULL_PTR, &digest_len));
  EXPECT_EQ(digestsize_, digest_len);

  // Provide too-small buffer => get too-small return code and the required length.
  digest_len = 2;
  EXPECT_CKR(CKR_BUFFER_TOO_SMALL,
             g_fns->C_DigestFinal(session_, buffer, &digest_len));
  EXPECT_EQ(digestsize_, digest_len);

  EXPECT_CKR_OK(g_fns->C_DigestFinal(session_, buffer, &digest_len));
}

TEST_P(DigestTest, DigestInvalid) {
  CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
  SKIP_IF_UNIMPLEMENTED_RV(rv);
  EXPECT_CKR_OK(rv);

  CK_BYTE buffer[512];
  EXPECT_CKR(CKR_ARGUMENTS_BAD,
             g_fns->C_Digest(session_, data_.get(), datalen_, buffer, NULL_PTR));

  /* TODO: reinstate
  // A call to C_Digest always terminates the active digest operation unless it
  // returns buffer-too-small/OK.
  CK_ULONG digest_len = sizeof(buffer);
  EXPECT_CKR(CKR_OPERATION_NOT_INITIALIZED,
             g_fns->C_Digest(session_, data_.get(), datalen_, buffer, &digest_len));
  */
}

TEST_P(DigestTest, DigestFinalInvalid) {
  CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
  SKIP_IF_UNIMPLEMENTED_RV(rv);
  EXPECT_CKR_OK(rv);

  // Digest and retrieve required length.
  CK_ULONG digest_len = 0;
  EXPECT_CKR_OK(g_fns->C_Digest(session_, data_.get(), datalen_, NULL_PTR, &digest_len));
  EXPECT_EQ(digestsize_, digest_len);

  // Attempt to finish with DigestFinal is not allowed.
  CK_BYTE buffer[512];
  digest_len = sizeof(buffer);
  EXPECT_CKR(CKR_OPERATION_ACTIVE,
             g_fns->C_DigestFinal(session_, buffer, &digest_len));

  /* TODO: reinstate
  // A failed DigestFinal should always terminate the active digest operation.
  digest_len = sizeof(buffer);
  EXPECT_CKR(CKR_OPERATION_NOT_INITIALIZED,
             g_fns->C_Digest(session_, data_.get(), datalen_, buffer, &digest_len));
  */
}

TEST_P(DigestTest, DigestUpdateNoInit) {
  EXPECT_CKR(CKR_OPERATION_NOT_INITIALIZED,
             g_fns->C_DigestUpdate(session_, data_.get(), 1));
}

TEST_P(DigestTest, DigestUpdateInvalidSession) {
  EXPECT_CKR(CKR_SESSION_HANDLE_INVALID, g_fns->C_DigestInit(INVALID_SESSION_HANDLE, &mechanism_));
}

TEST_P(DigestTest, DigestUpdateInvalid) {
  CK_RV rv = g_fns->C_DigestInit(session_, &mechanism_);
  SKIP_IF_UNIMPLEMENTED_RV(rv);
  EXPECT_CKR_OK(rv);

  EXPECT_CKR_OK(g_fns->C_DigestUpdate(session_, data_.get(), 0));
  EXPECT_CKR_OK(g_fns->C_DigestUpdate(session_, data_.get(), 1));

  // Now we're in incremental mode, an attempt to all-at-once digest should fail.
  CK_BYTE buffer[512];
  CK_ULONG digest_len = sizeof(buffer);
  EXPECT_CKR(CKR_OPERATION_ACTIVE,
             g_fns->C_Digest(session_, data_.get(), datalen_, buffer, &digest_len));

  digest_len = sizeof(buffer);
  EXPECT_CKR(CKR_SESSION_HANDLE_INVALID,
             g_fns->C_DigestFinal(INVALID_SESSION_HANDLE, buffer, &digest_len));

  digest_len = 1;
  EXPECT_CKR(CKR_BUFFER_TOO_SMALL,
             g_fns->C_DigestFinal(session_, buffer, &digest_len));
  EXPECT_EQ(digestsize_, digest_len);

  digest_len = sizeof(buffer);
  EXPECT_CKR_OK(g_fns->C_DigestFinal(session_, buffer, &digest_len));
}

TEST_P(DigestTest, TestVectors) {
  map<string, string> test_vector = kTestVectors[GetParam()];
  for (const auto& datum : test_vector) {
    const string& input = datum.first;
    const string& hex_expected = datum.second;
    string actual = PerformDigest(input);
    SKIP_IF_UNIMPLEMENTED(actual);
    string hex_actual = hex_data(actual);
    EXPECT_EQ(hex_expected, hex_actual) << " for input '" << input << "'";
  }
}

INSTANTIATE_TEST_CASE_P(Digests, DigestTest,
                        ::testing::Values(CKM_MD5, CKM_SHA_1, CKM_SHA256, CKM_SHA384, CKM_SHA512));

}  // namespace test
}  // namespace pkcs11
