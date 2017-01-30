#pragma once

/*
 * Class for encapsulating workspace paths.
 *
 * A workspace path is rooted with the user's identity (/user@patricbrc.org)
 * followed by a workspace name and then the path within the workspace.
 *
 * We store the path as a vector of components that we concatenate to form
 * readable paths as necessary.
 */

#include <deque>
#include <string>
#include <iostream>
#include <boost/algorithm/string/join.hpp>

class WsPath
{
public:
    WsPath() {}
    WsPath(const std::string &p);
    WsPath(WsPath &p);

    friend std::ostream &operator<<(std::ostream &out, const WsPath &p) {
	out << p.to_string();
	return out;
    }
    operator std::string() const { return to_string(); }
    std::string to_string() const;

    const std::string &user () const { return user_; }
    const std::string &workspace () const { return workspace_; }

    std::string delimited_path() const { return boost::join(path_, "/"); }

private:
    std::string user_;
    std::string workspace_;
    std::deque<std::string> path_;
};
