#include "ws_path.h"
#include <boost/algorithm/string.hpp>

namespace alg = boost::algorithm;

WsPath::WsPath(const std::string &p)
{
    if (p[0] != '/')
	throw std::runtime_error("WS paths must be absolute");

    alg::split(path_, p, alg::is_any_of("/"), alg::token_compress_on);;

    std::cerr << "Parse '" << p << "'\n";

    if (path_.size())
	path_.pop_front();
    if (path_.size())
    {
	user_ = path_.front();
	path_.pop_front();
    }
    if (path_.size())
    {
	workspace_ = path_.front();
	path_.pop_front();
    }

    if (path_.size() && path_.back() == "")
    {
	std::cerr << "isdir\n";
	path_.pop_back();
    }

    if (path_.size())
    {
	name_ = path_.back();
	path_.pop_back();
    }

    std::cerr << "user: " << user_ << "\n";
    std::cerr << "ws: " << workspace_ << "\n";

    for (auto x: path_)
    {
	std::cerr << "'" << x << "'\n";
    }
    std::cerr << "name: '" << name_ << "'\n";
}

WsPath::WsPath(WsPath &p) :
    user_(p.user_),
    workspace_(p.workspace_),
    path_(p.path_)
{
}
    


std::string WsPath::to_string() const
{
    std::string out("/");
    if (!user_.empty())
    {
	out.append(user_);
	out.append("/");
    }
    if (!workspace_.empty())
    {
	out.append(workspace_);
    }

    for (auto it: path_)
    {
	out.append("/");
	out.append(it);
    }
    return out;
}
