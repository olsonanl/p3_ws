#pragma once

#include <string>
#include <boost/filesystem.hpp>

#include "curl_aio.h"
#include "auth_token.h"

namespace fs = boost::filesystem;

class AuthMgr
{
public:
    AuthMgr(libcurl_wrapper::CurlAIO &curl_aio, const std::string &cache_dir);
    ~AuthMgr();

    AuthToken create_token(const std::string &txt);
    bool validate(AuthToken &token);

    std::string pubkey_for_signing_subject(const std::string &signing_subject);
    std::string find_pubkey_in_cache(const std::string &signing_subject);

private:
    libcurl_wrapper::CurlAIO &curl_aio_;
    fs::path cache_directory_;
};
