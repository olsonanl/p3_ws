
#include "kserver.h"

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include "global.h"

RequestServer::RequestServer(boost::asio::io_service& io_service,
			     const std::string &port,
			     const std::string &port_file
    ) :
    io_service_(io_service),
    acceptor_(io_service_),
    signals_(io_service_),
    port_(port),
    port_file_(port_file)
{
    
}

void RequestServer::register_path(const std::string &method,
				  const std::string &path,
				  request_callback_t cb)
{
    http_method_registry_[method].push_back(path_callback { path, cb });
}

void RequestServer::process_request(RequestPtr req)
{
    for (auto ent : http_method_registry_[req->request_type()])
    {
	// std::cerr << "compare '" << req->path() << "' '" << ent.path << "'\n";
	if (req->path().compare(0, ent.path.length(), ent.path) == 0)
	{
	    ent.callback(req);
	}
    }
}
    

/*
 * Need to split startup code from constructor due to use of enable_shared_from_this().
 */
void RequestServer::startup()
{

    /*
     * Set up for clean signal handling / termination
     */
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    signals_.add(SIGQUIT);
    do_await_stop();

    /*
     * Set up listener
     */

    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({"0.0.0.0", port_});
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    std::cerr << "Listening on " << acceptor_.local_endpoint() << "\n";
    if (!port_file_.empty())
    {
	std::ofstream out(port_file_);
	out << acceptor_.local_endpoint().port() << "\n";
	out.close();
    }
	    
    do_accept();
}

void RequestServer::do_accept()
{
    RequestPtr r = std::make_shared<Request>(shared_from_this(),
							   io_service_);
    // std::cerr << "create " << r << " in do_accept\n";
    acceptor_.async_accept(r->socket(),
			   boost::bind(&RequestServer::on_accept, this,
				       boost::asio::placeholders::error, r));
    // std::cerr << "leaving do_accept r use count=" << r.use_count() << "\n";
}

void RequestServer::on_accept(boost::system::error_code ec, RequestPtr r)
{
    // std::cerr << "on-accept use=" << r.use_count() << "\n";
    g_timer.start();
    // Check whether the server was stopped by a signal before this
    // completion handler had a chance to run.
    if (!acceptor_.is_open())
    {
	std::cerr << "not open\n";
	return;
    }
    
    if (!ec)
    {
	/*
	 * Connection has come in.
	 * Begin parsing the request line and headers.
	 */

	r->do_read();
    }
    
    do_accept();
}

void RequestServer::do_await_stop()
{
    signals_.async_wait(
	[this](boost::system::error_code ec, int signo)
	{
	    std::cerr << "Exiting with signal " << signo << "\n";
	    acceptor_.close();
	});
}

