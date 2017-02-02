#include "ws.h"
#include "ws_client.h"
#include "ws_item.h"

#include <list>

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

Workspace::Workspace(const std::string &host, int port, const std::string &database) :
    host_(host),
    port_(port),
    database_name_(database)
{
    mongocxx::uri uri("mongodb://" + host_ + ":" + std::to_string(port_));
    std::cerr << "connect to " << uri.to_string() << "\n";
    client_ = mongocxx::client(uri);
    if (!client_)
    {
	std::cerr << "error connecting to " << uri.to_string() << "\n";
    }
    std::cerr << client_.uri().to_string() << "\n";

    db_ = client_.database(database_name_);
    std::cerr << "name: " << db_.name() << "\n";
    std::cerr << "coll: " << db_.has_collection("objects") << "\n";
}

void Workspace::test(const WsPath &p)
{
    auto coll = db_["workspaces"];

    std::cerr<< "ws is " << p.workspace() << "\n";
    auto q = document{};
    q << "owner" << p.user();
    if (!p.workspace().empty())
    {
	q << "name" << p.workspace();
    }
    auto val = q << finalize;
    // std::cerr << "query doc: " << bsoncxx::to_json(val) << "\n";
    mongocxx::cursor cursor = coll.find(val.view());

    WsItemSet res(std::move(cursor));

    std::cerr << "Query\n";
    for (auto x : res)
    {
	std::cerr << x;
    }
    std::cerr << "Query done\n";
}

/*
 * Query the mongo db for the workspaces that are visible to this user.
 */

WsItemSet Workspace::query_workspaces(UserContext &ctx)
{
    auto coll = db_["workspaces"];

    auto q = document{};
    auto val = q << "$or" 
		 << open_array
		 << open_document << "owner" << ctx.username() << close_document
		 << open_document << "global_permission" << open_document << "$ne" << "n" << close_document << close_document
		 << close_array
		 << finalize;

    // std::cerr << "query doc: " << bsoncxx::to_json(val) << "\n";
    mongocxx::cursor cursor = coll.find(val.view());

    return WsItemSet(std::move(cursor), "workspace");
}

boost::optional<std::string> Workspace::lookup_workspace_id(UserContext &ctx, const WsPath &path)
{
    auto coll = db_["workspaces"];

    auto q = document{};
    auto val = q << "owner" << path.user()
		 << "name" << path.workspace()
		 << "$or" 
		 << open_array
		 << open_document << "owner" << ctx.username() << close_document
		 << open_document << "global_permission" << open_document << "$ne" << "n" << close_document << close_document
		 << close_array
		 << finalize;
    auto uproj = document{} << "uuid" << 1 << finalize;

    // std::cerr << "query doc: " << bsoncxx::to_json(val) << "\n";
    mongocxx::options::find o;
    o.projection(uproj.view());
    mongocxx::cursor cursor = coll.find(val.view(), o);

    auto item = cursor.begin();
    if (item == cursor.end())
    {
	return boost::none;
    }

    bsoncxx::document::view x = *item;
    std::cerr << "got " << bsoncxx::to_json(*item) << "\n";
    auto uuid = x["uuid"].get_utf8().value.to_string();
    return uuid;
}

boost::optional<WsItem> Workspace::query_object(UserContext &ctx, const WsPath &path, bool metadata_only)
{
    auto coll = db_["objects"];

    auto ws_id = lookup_workspace_id(ctx, path);
    if (!ws_id)
    {
	std::cerr << "No ws found for " << ctx <<  " " << path << "\n";
	return boost::none;
    }

    auto val = document{} << "path" << path.delimited_path()
			  << "name" << path.name() 
			  << "$or" 
			  << open_array
			  << open_document << "owner" << ctx.username() << close_document
			  << close_array
			  << finalize;

    std::cerr << "query doc: " << bsoncxx::to_json(val) << "\n";
    mongocxx::stdx::optional<bsoncxx::document::value> res = coll.find_one(val.view());
    if (res)
    {
	std::cerr << "return: " << bsoncxx::to_json(*res) << "\n";
	boost::optional<WsItem> item(WsItem(std::move(*res)));

	if (!metadata_only)
	{
	    /*
	     * TODO fill in file data from filesystem or shock node.
	     */
	    item->fill_data();
	}
	return item;
    }
    else
    {
	return boost::none;
    }
}

/*
 * End-user API routine.
 *
 * Here we need to make multiple requestss because the object list may
 * have multiple workspaces each of which need to have permissions lookups
 * done. This can likely be optimized in future if it is a performance
 * bottleneck to make the multiple requests to mongo to retrieve the data,
 * but I suspect typical usage has a small number of objects in the list.
 */
std::vector<WsItem> Workspace::get(UserContext &ctx, std::vector<WsPath> objects, bool adminmode)
{
    std::vector<WsItem> rval;
    for (auto path: objects)
    {
	boost::optional<WsItem> res = query_object(ctx, path, adminmode);
	if (res)
	{
	    rval.push_back(std::move(*res));
	}
    }
    return rval;
}



/*
json Workspace::ls(const list_params &params)
{
    for (auto p: params.paths)
    {
	std::cerr << "Process " << p << "\n";
    }

    json retval (R"(["abc"])"_json);
    retval.push_back(params.adminmode);
    return retval;
}

*/
