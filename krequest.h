#ifndef _KREQUEST_H
#define _KREQUEST_H

#include <vector>
#include <string>
#include <set>
#include <memory>
#include <boost/asio.hpp>
#include <boost/timer/timer.hpp>
#include <ostream>

class RequestServer;

class Request : public std::enable_shared_from_this<Request>
{
public:
    Request(std::shared_ptr<RequestServer> server, boost::asio::io_service &io_service);
    ~Request();

    void do_read();

    boost::asio::io_service &io_service()
    {
	return io_service_;
    }

    boost::asio::ip::tcp::socket& socket()
    {
	return socket_;
    }

    boost::asio::streambuf &request()
    {
	return request_;
    }

    std::map<std::string, std::string> &parameters()
    {
	return parameters_;
    }

    std::map<std::string, std::string> &headers()
    {
	return headers_;
    }

    void write_header(std::ostream &os, int code, const std::string &status);

    const std::string &request_type() { return request_type_; }
    const std::string &path() { return path_; }

    void read_body(std::function<void(boost::asio::streambuf &sb)> on_data,
		   std::function<void()> on_complete);

    void respond(int code, const std::string &status, const std::string &result,
		 std::function<void()> on_done);

private:
    void read_initial_line(boost::system::error_code err, size_t bytes);
    void read_headers(boost::system::error_code err, size_t bytes);
    void on_data(boost::system::error_code err, size_t bytes, std::function<void(boost::asio::streambuf &sb)> on_data_cb,
		   std::function<void()> on_complete_cb);

    void process_request();

    std::shared_ptr<RequestServer> server_;

    std::string request_type_;
    std::string path_;
    std::string parameters_raw_;
    std::string fragment_;
    std::string http_version_;
    std::map<std::string, std::string> parameters_;
    std::map<std::string, std::string> headers_;

    int content_length_;

    boost::asio::io_service &io_service_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
};


#endif
