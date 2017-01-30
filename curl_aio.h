/*
 * Async IO interface to libcurl.
 */

#pragma once

#include <string>
#include <curl/curl.h>
#include <functional>
#include <list>
#include <set>
#include <memory>
#include <boost/asio.hpp>

namespace libcurl_wrapper
{

class CurlAIO;
typedef std::function<void(const std::string &hdr, const std::string &value)> header_cb_t;
typedef std::function<void(char *data, size_t length)> data_cb_t;
typedef std::function<void()> complete_cb_t;

class RequestWrapper : public std::enable_shared_from_this<RequestWrapper>
{
public:
    friend class CurlAIO;

    RequestWrapper(CURL *curl, CurlAIO &aio, header_cb_t, data_cb_t, complete_cb_t);

    ~RequestWrapper() {
	// std::cerr << "Destroy RW \n";
    }

    CurlAIO &aio() { return aio_; }
    
    static size_t write_data_callback_s(char *ptr, size_t size, size_t nmemb, void *userdata);
    size_t write_data_callback(char *ptr, size_t size, size_t nmemb);

private:
    CURL *curl_;
    CurlAIO &aio_;

    header_cb_t header_cb_;
    data_cb_t data_cb_;
    complete_cb_t complete_cb_;

    static size_t write_cb(char *ptr, size_t size, size_t nmemb, RequestWrapper *rw_raw);
    static size_t header_cb(char *ptr, size_t size, size_t nmemb, RequestWrapper *rw_raw);
    static int prog_cb(void *p, double dltotal, double dlnow, double ult, double uln);
};

class CurlAIO
{
public:
    CurlAIO(boost::asio::io_service &io_service);
    ~CurlAIO();

    static void global_init();

    void request(const std::string &url, header_cb_t header_cb, data_cb_t data_cb, complete_cb_t complete_cb);
    void request(const std::string &url, std::function<void(std::shared_ptr<std::string> content)>);
    std::string request(const std::string &url);

    void monitor_socket(curl_socket_t sock, std::shared_ptr<boost::asio::ip::tcp::socket> sobj) {
	socket_map_[sock] = sobj;
    }
    void unmonitor_socket(curl_socket_t sock) {
	auto it = socket_map_.find(sock);
	if (it != socket_map_.end())
	{
	    socket_map_.erase(it);
	}
    }

    /*
    void dump_sockmap()
    {
	std::cerr << "Socket map:\n";
	for (auto x: socket_map_)
	{
	    std::cerr << x.first << " " << x.second.get() << "\n";
	}
    }
    */
	
    boost::asio::io_service &io_service() { return io_service_; }

private:
    CURLM *curl_handle_;
    boost::asio::io_service &io_service_;
    boost::asio::deadline_timer timer_;

    static size_t write_data_callback(void *ptr, size_t size, size_t nmemb, void *stream);

    static curl_socket_t open_socket_cb(void *clientp,
					curlsocktype purpose,
					struct curl_sockaddr *address);
    static int close_socket_cb(void *clientp, curl_socket_t item);

    static int socket_cb(CURL *easy, curl_socket_t socket, int what, void *userp, void *socketp);

    static int multi_timer_cb(CURLM *multi, long timeout_ms, CurlAIO *aio);
    static void timer_cb(const boost::system::error_code &error, CurlAIO *aio);
    void event_cb(std::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket,
		  int action, const boost::system::error_code & error, int *fdp);

    void check_multi_info();

    void poll_add_socket(curl_socket_t socket, CURL *easy, int action);
    void poll_remove_socket(int *f);
    void poll_set_socket(int *fdp, curl_socket_t s, CURL *easy, int action, int old_action);
    
    std::set<std::shared_ptr<RequestWrapper>> active_requests_;
    std::map<curl_socket_t, std::shared_ptr<boost::asio::ip::tcp::socket>> socket_map_;

    int still_running_;
};

};
