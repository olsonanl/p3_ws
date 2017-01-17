#include "ws_client.h"
#include "ws.h"

WorkspaceClient::WorkspaceClient(Workspace &ws) : ws_(ws)
{
}

json WorkspaceClient::handle_call(const std::string &call, const json &params)
{
    std::cerr << "Workspace handling call to " << call << ": " << params << "\n";

    if (call == "ls")
    {
	return ws_.ls(list_params(params[0]));
    }
    else
    {
	return "null"_json;
    }
}
