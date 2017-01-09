#ifndef _KSERVER_H
#define _KSERVER_H


/*
 * Request server.
 *
 * We speak pidgin HTTP here so we can make this available over a proxy.
 * This is NOT a general purpose HTTP server.
 *
 */

#include <boost/asio.hpp>
#include <set>
#include <memory>
#include "krequest.h"

class RequestServer : public std::enable_shared_from_this<RequestServer>
{
public:
    RequestServer(boost::asio::io_service& io_service,
		  const std::string &port,
		  const std::string &port_file);

    void startup();

    typedef std::shared_ptr<Request> RequestPtr;

private:

    void do_accept();
    void on_accept(boost::system::error_code ec, RequestPtr);
    
    void do_await_stop();

    boost::asio::io_service &io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    std::string port_;
    std::string port_file_;
};


#endif
