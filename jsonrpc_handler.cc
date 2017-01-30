#include "jsonrpc_handler.h"

#include <iostream>
#include <memory>


class Buf
{
public:
    Buf(size_t len) {
//	std::cerr << "alloc buf " << len << "\n";
	dat = new char[len];
	cur = dat;
    };

    Buf(const Buf&) = delete;

    void dealloc() {
//	std::cerr << "dealloc buf\n";
	if (dat)
	{
	    delete dat;
	    dat = 0;
	    cur = 0;
	}
    };

    ~Buf() {
//	std::cerr << "destroy buf\n";
	dealloc();
    };

    
    char *dat;
    char *cur;
};

JsonRpcHandler::JsonRpcHandler()
{
    // std::cerr << "construct JsonRpcHandler\n";
}

void JsonRpcHandler::handle_request(RequestPtr request)
{
    auto x = request->headers().find("content-length");

    if (x == request->headers().end())
    {
	request->respond(500, "Missing content length", "Missing content length header\n", [](){ });
	return;
    }

    /*
     * Here we are keeping the buffer management entirely local
     * to this set of lambdas for clarity. Allocate the buffer
     * when we come in, append the data as the packets arrive.
     * When complete, parse the json, discard the string buffer,
     * and handle the request.
     */
    int content_length = std::stoi(x->second);
    std::shared_ptr<Buf> buf = std::make_shared<Buf>(content_length + 1);
    request->read_body([buf](boost::asio::streambuf &sb) {
	    boost::asio::streambuf::const_buffers_type bufs = sb.data();
		
	    // std::cerr << "got data chunk " << sb.size() << "\n";

	    std::copy(buffers_begin(bufs), buffers_end(bufs), buf->cur);
	    buf->cur += sb.size();
	},
	[this, buf, request](){
	    *(buf->cur) = '\0';

	    // std::cerr << "done with data " << (buf->cur - buf->dat) << "\n";
	    try {
		 json j = json::parse(buf->dat);
		 buf->dealloc();
		 handle_parsed_request(request, j);
	    } catch (std::invalid_argument &e) {
		std::cerr << "Failed parse: " << e.what() << "\n";
	    }

	});
};

void JsonRpcHandler::handle_parsed_request(RequestPtr request, json &data)
{
    std::cerr << "handle parsed " << data << "\n";

    json d = data["method"];
    if (!d.is_string())
    {
	std::cerr << "missing method\n";
	return;
    }
    const std::string &method = d;

    size_t dot = method.find(".");
    if (dot == std::string::npos)
    {
	std::cerr << "invalid method\n";
	return;
    }
    std::string module = method.substr(0, dot);
    std::string call = method.substr(dot+1);
    std::cerr << "method='" << method << "' module='" << module << "' call='" << call << "'\n";
    
    json result = invoke(module, call, data["params"]);

    json retval;
    retval["id"] = data["id"];
    retval["result"] = result;
    if (data["jsonrpc"] == "2.0")
    {
	retval["jsonrpc"] = data["jsonrpc"];
    }
    else
    {
    }
    std::cerr << "return: " << retval << "\n";

    request->respond(200, "OK", retval.dump(), [](){std::cerr << "response sent\n"; });
}

void JsonRpcHandler::register_module(const std::string &module, method_handler_t h)
{
    module_registry_[module] = h;
}

json JsonRpcHandler::invoke(const std::string &module, const std::string &call, const json &params)
{
    auto handler = module_registry_[module];
    if (handler)
    {
	json retval = handler(call, params);
	return retval;
    }
}
