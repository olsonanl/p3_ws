#ifndef _USER_CONTEXT_H
#define _USER_CONTEXT_H

/*
 * A user context defines the important per-user information;
 * this includes the path to the home workspace for the user
 * and an enumeration of any workspaces that have been shared
 * with this user.
 *
 * We initialize a user context with an authorization token.
 */

class UserContext
{
public:
    UserContext(const std::string &token);
}


#endif /*  _USER_CONTEXT_H */
