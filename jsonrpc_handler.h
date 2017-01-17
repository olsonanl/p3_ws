#ifndef JSONRPC_HANDLER_H
#define JSONRPC_HANDLER_H

#include <map>
#include <functional>

#include "kserver.h"
#include "krequest.h"

#include "json.hpp"

using json = nlohmann::json;

typedef std::function<json(const std::string &call, const json&params)> method_handler_t;

class JsonRpcHandler
{
public:
    JsonRpcHandler();
    JsonRpcHandler(const JsonRpcHandler &) = delete;

    void handle_request(RequestPtr request);
    void handle_parsed_request(RequestPtr request, json &data);
    json invoke(const std::string &module, const std::string &call, const json &params);
    
    void register_module(const std::string &module, method_handler_t h);

    private:

    std::map<std::string, method_handler_t> module_registry_;

};

#endif /* JSONRPC_HANDLER_H */
