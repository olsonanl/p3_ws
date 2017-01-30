#include "user_context.h"

UserContext::UserContext(const AuthToken &tok) :
    user_token_(tok)
{
}

std::ostream &operator<<(std::ostream &out, UserContext &uc)
{
    out << "UserContext<" << uc.user_token_ << ">";
    return out;
}
