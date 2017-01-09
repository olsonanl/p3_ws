#include "krequest.h"
#include <iostream>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/array.hpp>
#ifdef BLCR_SUPPORT
#include "libcr.h"
#endif

#include "global.h"
#include "debug.h"

const boost::regex request_regex("^([A-Z]+) ([^?#]*)(\\?([^#]*))?(#(.*))? HTTP/(\\d+\\.\\d+)");
/* $1 = type
 * $2 = path
 * $4 = parameters
 * $6 = fragment
 */
const boost::regex mapping_path_regex("^/mapping/([^/]+)(/(add|matrix|lookup))$");

Request::Request(std::shared_ptr<RequestServer> server,
		 boost::asio::io_service &io_service)
  : server_(server),
    io_service_(io_service),
    socket_(io_service_),
    request_(65536)
{
    std::cerr << "construct krequest " << this << "\n";
}

Request::~Request()
{
    std::cerr << "destroy krequest " << this << "\n";
}
			   
/*
 * Start processing our request.
 *
 * This was invoked when the acceptor completed the new connection on our socket_.
 *
 */
void Request::do_read()
{
    std::cerr << "do_read " << this << "\n";
    auto obj = shared_from_this();
    boost::asio::async_read_until(socket_, request_, "\n",
				  [obj](boost::system::error_code err, size_t bytes_transferred)
				  {
				      obj->read_initial_line(err, bytes_transferred);
				  });
}

			   
std::string make_string(boost::asio::streambuf& streambuf)
{
    return {buffers_begin(streambuf.data()),
	    buffers_end(streambuf.data())};
}

void stringPurifier ( std::string& s )
{
    for ( std::string::iterator it = s.begin(), itEnd = s.end(); it!=itEnd; ++it)
    {
	if ( static_cast<unsigned int>(*it) < 32 || static_cast<unsigned int>(*it) > 127 )
	{
	    (*it) = ' ';
	}
    }
}

void Request::read_initial_line(boost::system::error_code err, size_t bytes)
{
    if (!err)
    {
	std::istream is(&request_);
	std::string line;
	if (std::getline(is, line))
	{
	    size_t s = line.find('\r');
	    if (s != std::string::npos)
	    {
		line.erase(s);
	    }
	    if (debug_is_set("debug-http"))
		std::cerr << "Request: " << this << " "  << line << "\n";
	    
	    boost::smatch match;
	    if (boost::regex_match(line, match, request_regex))
	    {
		request_type_ = match[1];
		path_ = match[2];
		parameters_raw_ = match[4];
		fragment_ = match[6];
		http_version_ = match[7];

		if (!parameters_raw_.empty())
		{
		    std::vector<std::string> parts;
		    boost::split(parts, parameters_raw_, boost::is_any_of(";&"));
		    for (auto it : parts)
		    {
			size_t pos = it.find('=');
			if (pos != std::string::npos)
			{
			    parameters_[it.substr(0, pos)] = it.substr(pos+1);
			}
		    }
		}

		// std::cerr << "Got " << request_type_ << " for path " << path_ << "\n";

		if (!is.eof())
		{
		    auto obj = shared_from_this();
		    boost::asio::async_read_until(socket_, request_, "\n",
						  [obj](boost::system::error_code err, size_t bytes)
						  {
						      obj->read_headers(err, bytes);
						  });
		}
		else
		{
		    // std::cerr << "eof\n";
		}
	    }
	    else
	    {
		std::cerr << "Invalid request '" << line << "'\n";
	    }
	}
	else
	{
	    std::cerr << "getline failed\n";
	}
    }
    else
    {
	std::cerr << "error " << err << "\n";
    }
    
}
void Request::read_headers(boost::system::error_code err, size_t bytes)
{
    // std::cerr << "read_headers " << this << " err=" << err << "\n";
    if (!err)
    {
	// std::string r = make_string(request_);
	// std::cerr << "Read buffer contains: "  << r << std::endl;

	std::istream is(&request_);
	std::string line;
	bool finished = false;
	while (!finished && std::getline(is, line))
	{
	    size_t s = line.find('\r');
	    if (s != std::string::npos)
	    {
		line.erase(s);
	    }
	    // std::cerr << "Got line '" << line << "'\n";

	    if (line != "")
	    {
		size_t x = line.find(':');
		std::string k(line.substr(0,x));
		x++;
		while (line[x] == ' ')
		    x++;
		std::string v(line.substr(x));
		std::transform(k.begin(), k.end(), k.begin(), ::tolower);
		headers_[k] = v;
	    }
	    else
	    {
		finished = true;
	    }
	}
	if (finished)
	{
	    if (debug_is_set("debug-http"))
	    {
		std::cerr << "Headers:\n";
		for (auto x: headers_)
		{
		    std::cerr << x.first << ": " << x.second << "\n";
		}
	    }
	    /*
	     * We don't support chunked encoding.
	     */
	    auto x = headers_.find("transfer-encoding");
	    if (x != headers_.end() && x->second == "chunked")
	    {
		respond(501, "Chunked encoding not implemented",
			"Chunked encoding not implemented\n",
			[this](){ });
		return;
	    }
	    try {
		process_request();
	    }
	    catch (std::exception &e)
	    {
		std::ostringstream o;
		o << "Caught exception " << e.what() << "\n";
		std::cerr << o.str();
		respond(500, "Failed", o.str(), [](){});
	    }
	    catch (...)
	    {
		std::ostringstream o;
		o << "Caught default exception\n";
		std::cerr << o.str();
		respond(500, "Failed", o.str(), [](){});
	    }
		    
	}
	else
	{
	    auto obj = shared_from_this();
	    boost::asio::async_read_until(socket_, request_, "\n",
					  [obj](boost::system::error_code err, size_t bytes)
					  {
					      obj->read_headers(err, bytes);
					  });
	}
    } 
    else if (err == boost::asio::error::eof)
    {
	// std::cerr << "eof\n";
    }
    else
    {
	std::cerr << "error " << err << "\n";
    }
}

void Request::process_request()
{
    std::cerr << "Process request type " << request_type_ << "\n";
    
    /*
     * Process Expect: 100-continue
     */

    auto it = headers_.find("expect");
    if (it != headers_.end() && it->second == "100-continue")
    {
	// std::cerr << "handling expect 100-continue\n";
	boost::asio::streambuf s;
	std::ostream os(&s);
	os << "HTTP/" << http_version_ << " 100 Continue\n\n";
	boost::asio::write(socket_, s);
    }

    respond(200, "OK", "OK\n", [this](){});
}

void Request::write_header(std::ostream &os, int code, const std::string &status)
{
    os << "HTTP/" << http_version_ << " " << code << " " << status << "\n";
    os << "Content-type: text/plain\n";
}

void Request::respond(int code, const std::string &status, const std::string &result, std::function<void()> on_done)
{
    std::ostringstream os;
    write_header(os, code, status);
    os << "Content-length: " << result.size() << "\n";
    os << "\n";

    std::vector<boost::asio::const_buffer> bufs;
    std::string s(os.str());
    bufs.push_back(boost::asio::buffer(s));
    bufs.push_back(boost::asio::buffer(result));

    //
    // Use the shared ptr here to keep this object alive through the chain of lambda invocations.
    //
    auto obj = shared_from_this();
    boost::asio::async_write(socket_, bufs,
			     [obj, on_done](const boost::system::error_code &err, const long unsigned int &bytes){
				 // std::cerr << "write all finished\n";
				 on_done();
			     });
}

