#include "auth_token.h"

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
	comps_[m.substr(0, loc)] = m.substr(loc + 1);
    }
}

void AuthToken::dump()
{
    for (auto item: comps_)
    {
	std::cerr << item.first << ": " << item.second << "\n";
    }
}

bool AuthToken::validate()
{
    return true;
}

int main()
{
    AuthToken at(R"(un=olson@patricbrc.org|tokenid=63cbfdfb-a28a-4135-b24d-4e1217319c54|expiry=1515696940|client_id=olson@patricbrc.org|token_type=Bearer|realm=patricbrc.org|SigningSubject=https://user.patricbrc.org/public_key|sig=12096102320ee7354df5242eccb20abae85c553710ac375d09efa11839e470909e4c940b297c6e8ceae5d13f75e6b913cc87a61bab0639b7d64e70b84737ddbf977d6687d80f16ec502989fdf0531f213dd3eb6afc9d60eaa8bb366fbdc0a0c565eae8d0d01c75d0ab22568b18cf485a45e2d41d82fbe3498b8c709cd0933012)");

    std::cerr << at.username() << "\n";
    return 0;
}
