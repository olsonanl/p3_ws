#ifndef _AUTH_TOKEN
#define _AUTH_TOKEN

#include <string>
#include <map>
#include <list>

class AuthToken
{
public:
    AuthToken(const std::string &token);

    std::string username() { return get_component("un"); }

    void dump();

    friend class AuthMgr;

    const std::string &get_component(const std::string &s) {
	return comps_[s];
    }

private:
    std::string token_string_;
    std::map<std::string, std::string> comps_;

    std::string signed_component_;

    void add_item(const std::string &);
};

std::ostream &operator<<(std::ostream &out, AuthToken &uc);

#endif /* _AUTH_TOKEN */
