#ifndef _KSERVER_H
#define _KSERVER_H


/*
 * Request server.
 *
 * We speak pidgin HTTP here so we can make this available over a proxy.
 * This is NOT a general purpose HTTP server.
 *
 */

#include <string>
#include <boost/asio.hpp>
#include <set>
#include <memory>
#include "krequest.h"

typedef std::shared_ptr<Request> RequestPtr;

class RequestServer : public std::enable_shared_from_this<RequestServer>
{
public:
    RequestServer(boost::asio::io_service& io_service,
		  const std::string &port,
		  const std::string &port_file);

    void startup();

    typedef std::function<void(RequestPtr req)> request_callback_t;
    struct path_callback
    {
	std::string path;
	request_callback_t callback;
    };

    void register_path(const std::string &method,
		       const std::string &path,
		       request_callback_t);

    void process_request(RequestPtr req);

private:

    void do_accept();
    void on_accept(boost::system::error_code ec, RequestPtr);
    
    void do_await_stop();

    boost::asio::io_service &io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    std::string port_;
    std::string port_file_;

    std::map<std::string, std::vector<path_callback>> http_method_registry_;
};


#endif
