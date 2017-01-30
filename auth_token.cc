#include "auth_token.h"
#include "auth_mgr.h"
#include "user_context.h"
#include "ws_path.h"
#include "curl_aio.h"
#include <iostream>

AuthToken::AuthToken(const std::string &token) : token_string_(token)
{
    size_t loc, prev_loc;
    prev_loc = 0;
    while ((loc = token.find('|', prev_loc)) != std::string::npos)
    {
	std::string m = token.substr(prev_loc, loc - prev_loc);
	add_item(m);
	prev_loc = loc + 1;
    }
    std::string m = token.substr(prev_loc);
    add_item(m);
}

void AuthToken::add_item(const std::string &m)
{
    size_t loc = m.find("=");
    if (loc != std::string::npos)
    {
	std::pair<std::string, std::string> x = std::make_pair(m.substr(0, loc), m.substr(loc + 1));
	comps_.insert(x);
	if (x.first != "sig")
	{
	    if (!signed_component_.empty())
		signed_component_.append("|");
	    signed_component_.append(m);
	}
    }
}

void AuthToken::dump()
{
    std::cerr << "map:\n";
    for (auto item: comps_)
    {
	std::cerr << item.first << ": " << item.second << "\n";
    }
}

std::ostream &operator<<(std::ostream &out, AuthToken &uc)
{
    out << "AuthToken<" << uc.username() << ">";
    return out;
}

#ifdef AUTH_TOKEN_MAIN

int main()
{
    std::string tstring(R"(un=olson@patricbrc.org|tokenid=63cbfdfb-a28a-4135-b24d-4e1217319c54|expiry=1515696940|client_id=olson@patricbrc.org|token_type=Bearer|realm=patricbrc.org|SigningSubject=https://user.patricbrc.org/public_key|sig=12096102320ee7354df5242eccb20abae85c553710ac375d09efa11839e470909e4c940b297c6e8ceae5d13f75e6b913cc87a61bab0639b7d64e70b84737ddbf977d6687d80f16ec502989fdf0531f213dd3eb6afc9d60eaa8bb366fbdc0a0c565eae8d0d01c75d0ab22568b18cf485a45e2d41d82fbe3498b8c709cd0933012)");

    libcurl_wrapper::CurlAIO::global_init();

    boost::asio::io_service io_service;
    libcurl_wrapper::CurlAIO curl_aio(io_service);

    AuthMgr auth_mgr(curl_aio, "cache");
    AuthToken at(auth_mgr.create_token(tstring));

    std::cerr << at.username() << "\n";

    UserContext uc(at);

    std::cerr << uc << "\n";

    std::list<std::string> t = {"/olson@patricbrc.org/foo/bar/baz",
				"/olson@patricbrc.org/foo/bar/",
				"/olson@patricbrc.org/foo/bar",
				"/olson@patricbrc.org/foo/",
				"/olson@patricbrc.org/",
				"/olson@patricbrc.org",
				"/"};
    for (auto s: t)
    {
	WsPath w(s);
	std::cerr << "final: '" << w << "'\n";
    }

    return 0;
}
#endif
