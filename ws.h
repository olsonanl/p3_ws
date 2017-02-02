#ifndef _WS_H
#define _WS_H

#include <boost/optional.hpp>
#include <mongocxx/client.hpp>

#include "json.hpp"
#include "ws_client.h"
#include "ws_path.h"
#include "ws_item.h"
#include "user_context.h"

using json = nlohmann::json;

class Workspace
{
public:
    Workspace(const std::string &host, int port, const std::string &database_name);
    void test(const WsPath &p);

    WsItemSet query_workspaces(UserContext &ctx);

    boost::optional<WsItem> query_object(UserContext &ctx, const WsPath &path, bool metadata_only);
    boost::optional<std::string> lookup_workspace_id(UserContext &ctx, const WsPath &path);

    /*
     * Routines that map to the API
     */
    std::vector<WsItem> get(UserContext &ctx, std::vector<WsPath> objects, bool adminmode);


private:
    std::string host_;
    int port_;
    std::string database_name_;
    
    mongocxx::client client_;
    mongocxx::database db_;
};


#endif /* _WS_H */
