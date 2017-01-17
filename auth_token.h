#ifndef _AUTH_TOKEN
#define _AUTH_TOKEN

#include <string>
#include <map>

class AuthToken
{
public:
    AuthToken(const std::string &token);

    bool validate();
    std::string username() { return comps_["un"]; }

    void dump();

private:
    std::string token_string_;
    std::map<std::string, std::string> comps_;

    void add_item(const std::string &);
};



#endif /* _AUTH_TOKEN */
