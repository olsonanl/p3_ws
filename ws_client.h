#ifndef _WS_CLIENT_H
#define _WS_CLIENT_H

#include <string>
#include <list>
#include <map>
#include "json.hpp"

using json = nlohmann::json;

class Workspace;

static bool soft_convert_bool(json t)
{
    return t.is_boolean() ? t.get<bool>() : (t.is_null() ? false : t.get<int>());
}



struct list_params
{
    std::vector<std::string> paths;
    bool excludeDirectories;
    bool excludeObjects;
    bool recursive;
    bool fullHierarchicalOutput;

    typedef std::map<std::string, std::vector<std::string>> map_t;
    map_t query;
    bool adminmode;

    list_params(json data) {
	paths = data["paths"].get<std::vector<std::string>>();

	excludeDirectories = soft_convert_bool(data["excludeDirectories"]);
	excludeObjects = soft_convert_bool(data["excludeObjects"]);
	recursive = soft_convert_bool(data["recursive"]);
	fullHierarchicalOutput = soft_convert_bool(data["fullHierarchicalOutput"]);
	for (json::iterator ent = data["query"].begin(); ent != data["query"].end(); ent++)
	{
	    query[ent.key()] = ent.value().get<std::vector<std::string>>();
	}
	adminmode = soft_convert_bool(data["adminmode"]);
    };
};

class WorkspaceClient
{
public:
    WorkspaceClient(Workspace &ws);
    json handle_call(const std::string &call, const json &params);

private:
    Workspace &ws_;
};


#endif /*  _WS_CLIENT_H */
