#pragma once

/*
 * A user context defines the important per-user information;
 * this includes the path to the home workspace for the user
 * and an enumeration of any workspaces that have been shared
 * with this user.
 *
 * We initialize a user context with an authorization token.
 */

#include "auth_token.h"
#include "ws_path.h"
#include <iostream>
#include <boost/filesystem.hpp>


class UserContext
{
public:
    UserContext(const AuthToken &tok);

    friend std::ostream &operator<<(std::ostream &out, UserContext &uc);

    std::string username() { return user_token_.username(); }
    WsPath user_home();

private:
    AuthToken user_token_;
    bool is_admin_;
};


