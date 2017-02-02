#include "auth_mgr.h"

#include <iostream>
#include <istream>

#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/sha.h>

#include "json.hpp"

using json = nlohmann::json;

AuthMgr::AuthMgr(libcurl_wrapper::CurlAIO &curl_aio, const std::string &cache_directory) :
    curl_aio_(curl_aio),
    cache_directory_(cache_directory)
{
    OpenSSL_add_all_algorithms();
}

AuthMgr::~AuthMgr()
{
    EVP_cleanup();
}

AuthToken AuthMgr::create_token(const std::string &txt)
{
    AuthToken tok(txt);
    if (!validate(tok))
	throw std::runtime_error("Invalid token");

    return tok;
}

bool AuthMgr::validate(AuthToken &token)
{
    std::string &to_sign(token.signed_component_);
    std::string token_signature(token.get_component("sig"));

    // convert hex signature to binary string

    std::string binary_signature;
    for (auto i = token_signature.begin(); i != token_signature.end(); i += 2)
    {
	char n[3];
	n[0] = i[0];
	n[1] = i[1];
	n[2] = 0;
	long int v = strtol(n, 0, 16);
	binary_signature.push_back((char) v);
    }


    /*
     *  Retrieve signing subject and associated public key.
     */
    std::string signing_subject(token.get_component("SigningSubject"));

    std::string pub = pubkey_for_signing_subject(signing_subject);
    // std::cerr << "got pubkey " << pub << "\n";

    /*
     * Use OpenSSL to compute hashes and verify signature.
     */

    unsigned char *sha = SHA1((const unsigned char *) to_sign.c_str(), to_sign.length(), 0);
    BIO *bio = BIO_new(BIO_s_mem());
    if (BIO_write(bio, pub.c_str(), pub.length()) <= 0)
	throw std::runtime_error("BIO_puts failed");

    RSA *rsa;
    EVP_PKEY* evp_key = 0;
    if (pub.find("BEGIN PUBLIC KEY") != std::string::npos)
    {
	evp_key = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
	rsa = EVP_PKEY_get1_RSA(evp_key);

    }
    else if (pub.find("BEGIN RSA PUBLIC KEY") != std::string::npos)
    {
	rsa = PEM_read_bio_RSAPublicKey(bio, 0, 0, 0);
    }
    else
    {
	throw std::runtime_error("Unknown signing key type in token");
    }
	
    if (!rsa)
	throw std::runtime_error("Error reading public key into RSA");
    
    int rc = RSA_verify(NID_sha1, sha, SHA_DIGEST_LENGTH,
			(const unsigned char *) binary_signature.c_str(), binary_signature.length(), rsa);

    BIO_free(bio);
    RSA_free(rsa);
    if (evp_key)
	EVP_PKEY_free(evp_key);
    return rc;
}

std::string AuthMgr::pubkey_for_signing_subject(const std::string &signing_subject)
{
    /*
     * Check cache.
     */

    std::string pub = find_pubkey_in_cache(signing_subject);
    if (!pub.empty())
	return pub;

    std::string pubj = curl_aio_.request(signing_subject);

    try {
	if (pubj.empty())
	{
	    throw(std::runtime_error("No text retrieved for signing subject"));
	}
	json j = json::parse(pubj);
	pub = j["pubkey"].get<std::string>();
	return pub;
	
    } catch (std::invalid_argument &e)
    {
	std::cerr << "Error parsing public key text:" << pubj << "\n";
	throw;
    }
}

std::string AuthMgr::find_pubkey_in_cache(const std::string &signing_subject)
{
    std::string cache_name = signing_subject;
    std::replace(cache_name.begin(), cache_name.end(), '/', '_');
    fs::path cfile = cache_directory_;
    cfile /= cache_name;

    if (fs::is_regular_file(cfile))
    {
	std::ifstream ifstr(cfile.string());
        std::string pub(
            (std::istreambuf_iterator<char>(ifstr)),
            std::istreambuf_iterator<char>()
        );
	return pub;
    }
    else
    {
	return "";
    }
}
