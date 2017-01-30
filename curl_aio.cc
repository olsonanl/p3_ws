#include <memory>
#include <iostream>
#include <tuple>
#include <stdio.h>
#include <thread>

#include "curl_aio.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/future.hpp>

using namespace libcurl_wrapper;

void CurlAIO::global_init()
{
    curl_global_init(CURL_GLOBAL_ALL);
}
	
CurlAIO::CurlAIO(boost::asio::io_service &io_service) :
    io_service_(io_service),
    timer_(io_service),
    still_running_(0)
{
    curl_handle_ = curl_multi_init();

    curl_multi_setopt(curl_handle_, CURLMOPT_SOCKETFUNCTION, socket_cb);
    curl_multi_setopt(curl_handle_, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(curl_handle_, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(curl_handle_, CURLMOPT_TIMERDATA, this);
    
}

CurlAIO::~CurlAIO()
{
    curl_multi_cleanup(curl_handle_);
}

void CurlAIO::request(const std::string &url,
		      std::function<void(std::shared_ptr<std::string> content)> cb)
{
    std::shared_ptr<std::string> accum = std::make_shared<std::string>();

    auto data_cb = [accum](char *str, size_t len) {
	accum->append(str, len);
	
    };
    auto completion_cb = [accum, cb] () {
	cb(accum);
    };
    request(url, 0, data_cb, completion_cb);
}

/*
 * Synchronous request that runs the aio event loop.
 */
std::string CurlAIO::request(const std::string &url)
{
    std::shared_ptr<std::string> result;
    bool ready = false;
    request(url, [this, &result, &ready](std::shared_ptr<std::string> x) {
	    result = x;
	    ready = true;
	});
    while (!ready)
    {
	io_service().run();
	io_service().reset();
    }
    return *result;
}

void CurlAIO::request(const std::string &url, header_cb_t header_cb, data_cb_t data_cb, complete_cb_t complete_cb)
{
    CURL *curl = curl_easy_init();

    if (!curl)
	throw std::runtime_error("curl_easy_init failed");

    std::shared_ptr<RequestWrapper> rw = std::make_shared<RequestWrapper>(curl, *this, header_cb, data_cb, complete_cb);
    
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, rw.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &RequestWrapper::write_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, rw.get());
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &RequestWrapper::header_cb);
    /*
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, rw.get());
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &RequestWrapper::prog_cb);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    */

    active_requests_.insert(rw);
    
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, open_socket_cb);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, this);

    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETFUNCTION, close_socket_cb);
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETDATA, this);

    curl_easy_setopt(curl, CURLOPT_PRIVATE, rw.get());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
    // curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1);

    CURLMcode rc = curl_multi_add_handle(curl_handle_, curl);
    if (rc != CURLM_OK)
    {
	std::cerr << "curl_multi_add_handle failed with code=" << rc << "\n";
	throw std::runtime_error("curlm failed");
    }
}

int CurlAIO::socket_cb(CURL *easy, curl_socket_t socket, int what, void *userp, void *socketp)
{
    CurlAIO *aio = static_cast<CurlAIO *>(userp);
    
    // std::cerr << "socket_cb what=" << what << " socket=" << socket << "\n";

    int *actionp = (int *) socketp;

    if (what == CURL_POLL_REMOVE)
    {
	// std::cerr << "  remove\n";
	aio->poll_remove_socket(actionp);
    }
    else
    {
	if (actionp == 0)
	{
	    // std::cerr << "  adding data\n";
	    aio->poll_add_socket(socket, easy, what);
	}
	else
	{
	    // std::cerr << "  changing action action from " << *actionp << " to " << what << "\n";
	    aio->poll_set_socket(actionp, socket, easy, what, *actionp);
	}
    }
    
    return 0;
}

void CurlAIO::poll_add_socket(curl_socket_t socket, CURL *easy, int action)
{
    int *fdp = new int;
    poll_set_socket(fdp, socket, easy, action, 0);
    curl_multi_assign(curl_handle_, socket, fdp);
}

void CurlAIO::poll_remove_socket(int *f)
{
    if (f)
	delete f;
}

void CurlAIO::poll_set_socket(int *fdp, curl_socket_t socket, CURL *easy,
			      int action, int old_action)
{
    auto it = socket_map_.find(socket);
    if (it == socket_map_.end())
    {
	// std::cerr << "socket " << socket << " not one of ours\n";
	return;
    }

    auto tcp_socket = it->second;

    if (fdp == 0)
	throw std::runtime_error("poll_set_socket got null fdp");
    *fdp = action;

    if (action == CURL_POLL_IN)
    {
	// std::cerr << "  watch for readable\n";
	if (old_action != CURL_POLL_IN && old_action != CURL_POLL_INOUT)
	{
	    tcp_socket->async_read_some(boost::asio::null_buffers(),
					boost::bind(&CurlAIO::event_cb, this, tcp_socket, CURL_POLL_IN, _1, fdp));
	}
    }
    else if (action == CURL_POLL_OUT)
    {
	// std::cerr << "  watch for writable\n";
	if (old_action != CURL_POLL_OUT && old_action != CURL_POLL_INOUT)
	{
	    tcp_socket->async_write_some(boost::asio::null_buffers(),
					boost::bind(&CurlAIO::event_cb, this, tcp_socket, CURL_POLL_OUT, _1, fdp));
	}
    }
    else if (action == CURL_POLL_INOUT)
    {
	// std::cerr << "  watch for readable and writable\n";
	if (old_action != CURL_POLL_IN && old_action != CURL_POLL_INOUT)
	{
	    tcp_socket->async_read_some(boost::asio::null_buffers(),
					boost::bind(&CurlAIO::event_cb, this, tcp_socket, CURL_POLL_IN, _1, fdp));
	}
	if (old_action != CURL_POLL_OUT && old_action != CURL_POLL_INOUT)
	{
	    tcp_socket->async_write_some(boost::asio::null_buffers(),
					boost::bind(&CurlAIO::event_cb, this, tcp_socket, CURL_POLL_OUT, _1, fdp));
	}
	
    }
}

void CurlAIO::timer_cb(const boost::system::error_code &error, CurlAIO *aio)
{
    // std::cerr << "timer_cb error=" << error << "\n";
    if (!error)
    {
	CURLMcode rc = curl_multi_socket_action(aio->curl_handle_, CURL_SOCKET_TIMEOUT,
						0, &aio->still_running_);
	if (rc != CURLM_OK)
	{
	    std::cerr << "curl_multi_socket_action failed with code=" << rc << "\n";
	    throw std::runtime_error("curl_multi_socket_action failed");
	}

	aio->check_multi_info();
    }
}

void CurlAIO::event_cb(std::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket,
		       int action, const boost::system::error_code & error, int *fdp)
{
//     std::cerr << "event_cb: action=" << action << "\n";

    if (*fdp == action || *fdp == CURL_POLL_INOUT)
    {
	curl_socket_t s = tcp_socket->native_handle();
	CURLMcode rc;
	if (error)
	    action = CURL_CSELECT_ERR;
	rc = curl_multi_socket_action(curl_handle_, s, action, &still_running_);
	if (rc != CURLM_OK)
	{
	    std::cerr << "curl_multi_socket_action failed with code=" << rc << "\n";
	    throw std::runtime_error("curl_multi_socket_action failed");
	}
	check_multi_info();

	if (still_running_ <= 0 && active_requests_.size() == 0)
	{
	    // std::cerr << "still_running_=" << still_running_ << " active req count " << active_requests_.size() << "\n";
	    // std::cerr << "last transfer done, kill timeout\n";
	    timer_.cancel();
	    return;
	}

	if (!error && socket_map_.find(s) != socket_map_.end() &&
	    (*fdp == action || *fdp == CURL_POLL_INOUT))
	{
	    if (action == CURL_POLL_IN)
	    {
		tcp_socket->async_read_some(boost::asio::null_buffers(),
					    boost::bind(&CurlAIO::event_cb, this, tcp_socket,
							action, _1, fdp));
	    }
	    if (action == CURL_POLL_OUT)
	    {
		tcp_socket->async_write_some(boost::asio::null_buffers(),
					     boost::bind(&CurlAIO::event_cb, this, tcp_socket,
							action, _1, fdp));
	    }
	}
    }
}


int CurlAIO::multi_timer_cb(CURLM *multi, long timeout_ms, CurlAIO *aio)
{
    // std::cerr << "multi_timer_cb " << timeout_ms << "\n";

    aio->timer_.cancel();
    if (timeout_ms > 0)
    {
	aio->timer_.expires_from_now(boost::posix_time::millisec(timeout_ms));
	aio->timer_.async_wait(boost::bind(&timer_cb, _1, aio));
    }
    else
    {
	boost::system::error_code error;
	timer_cb(error, aio);
    }
    
    return 0;
}

/* Check for completed transfers, and remove their easy handles */

void CurlAIO::check_multi_info()
{
    CURLMsg *msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(curl_handle_, &msgs_left)))
    {
	// std::cerr << "got msg " << msg->msg << "\n";
	if (msg->msg == CURLMSG_DONE)
	{
	    CURL *easy = msg->easy_handle;
	    RequestWrapper *rw_raw;
	    char *eff_url;
	    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &rw_raw);
	    curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
	    
	    std::shared_ptr<RequestWrapper> rw = rw_raw->shared_from_this();
	    
	    // std::cerr << "done: " << rw.use_count() << " " << eff_url << "\n";

	    if (rw_raw->complete_cb_)
		rw_raw->complete_cb_();

	    curl_multi_remove_handle(curl_handle_, easy);
	    auto it = active_requests_.find(rw);
	    if (it != active_requests_.end())
	    {
		// std::cerr << "removing from active\n";
		active_requests_.erase(it);
		// dump_sockmap();
	    }
	    else
	    {
		// std::cerr << "cannot find request!\n";
	    }
	    curl_easy_cleanup(easy);
	}
    }
}

curl_socket_t CurlAIO::open_socket_cb(void *clientp,
				      curlsocktype purpose,
				      struct curl_sockaddr *address)
{
    CurlAIO *aio = static_cast<CurlAIO *>(clientp);

    // std::cerr << "open socket cb\n";

    curl_socket_t sockfd = CURL_SOCKET_BAD;

    if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
    {
	// std::cerr << "create ipv4 socket\n";

	auto tcp_socket = std::make_shared<boost::asio::ip::tcp::socket>(aio->io_service());
	boost::system::error_code ec;
	tcp_socket->open(boost::asio::ip::tcp::v4(), ec);
	if (ec)
	{
	    std::cerr << "couldn't open socket: " << ec << " " << ec.message() << "\n";
	}
	else
	{
	    sockfd = tcp_socket->native_handle();
	    // std::cerr << "opened socket " << sockfd << "\n";
	    aio->monitor_socket(sockfd, tcp_socket);
	}
    }


    return sockfd;
}

int CurlAIO::close_socket_cb(void *clientp, curl_socket_t item)
{
    CurlAIO *aio = static_cast<CurlAIO *>(clientp);
    // std::cerr << "close socket cb\n";

    aio->unmonitor_socket(item);

    return 0;
}

RequestWrapper::RequestWrapper(CURL *curl, CurlAIO &aio, header_cb_t header_cb, data_cb_t data_cb, complete_cb_t complete_cb) :
    curl_(curl),
    aio_(aio),
    header_cb_(header_cb),
    data_cb_(data_cb),
    complete_cb_(complete_cb)
{
    
}

int RequestWrapper::prog_cb(void *p, double dltotal, double dlnow, double ult, double uln)
{
    RequestWrapper *rw = static_cast<RequestWrapper *>(p);

    std::cerr << rw << " " << dlnow << " " << dltotal << "\n";

    return 0;
}

size_t RequestWrapper::write_cb(char *ptr, size_t size, size_t nmemb, RequestWrapper *rw_raw)
{
//    std::cerr << "write data " << size << " " << nmemb << "\n";
    if (rw_raw->data_cb_)
	rw_raw->data_cb_(ptr, size * nmemb);
//    std::cerr << s << "\n";
    return size * nmemb;
}

size_t RequestWrapper::header_cb(char *ptr, size_t size, size_t nmemb, RequestWrapper *rw_raw)
{
    // std::cerr << "header data " << " " << size << " " << nmemb << "\n";

    if (rw_raw->header_cb_)
    {
	std::string s(ptr, size * nmemb);
	
	size_t pos = 0;
	while (pos != std::string::npos)
	{
	    size_t eol = s.find('\n', pos);
	    std::string line;
	    if (eol == std::string::npos)
	    {
		line = s.substr(pos);
		pos = eol;
	    }
	    else
	    {
		line = s.substr(pos, eol - pos - 1);
		pos = eol + 1;
	    }
	    size_t dot = line.find(':');
	    if (dot != std::string::npos)
	    {
		std::string hdr = line.substr(0, dot);
		dot++;
		while (std::isspace(line[dot]))
		    dot++;
		std::string value = line.substr(dot);

		rw_raw->header_cb_(hdr, value);
		// std::cerr << "hdr='" << hdr << "' value='" << value << "'\n";
	    }
	}
    }

    return size * nmemb;
}

#ifdef CURL_AIO_MAIN

int main()
{
    CurlAIO::global_init();

    boost::asio::io_service io_service;
    
    CurlAIO c(io_service);

//    boost::asio::io_service::work work(io_service);

#if 0
    for (int i = 0; i < 5; i++)
    {
	auto hcb =[i](const std::string &hdr, const std::string &value) {
	    std::cerr << i << ": " << hdr << "=" << value << "\n";
	};
	auto dcb = [i](char *data, size_t length) {
	    std::cerr << i << " data " << length << "\n";
	};
	auto ccb = [i]() {
	    std::cerr << i << " done\n"; 
	};

	c.request("http://bioseed.mcs.anl.gov/~olson/vir-human.2014-0414.tgz", hcb, 0, ccb);
	c.request("http://bioseed.mcs.anl.gov/~olson/test.txt", hcb, 0, ccb);
	c.request("http://bioseed.mcs.anl.gov/~olson/pseed.update-2012-0218.tgz", hcb, 0, ccb);
    }
#endif

    c.request("http://bioseed.mcs.anl.gov/~olson/", [](std::shared_ptr<std::string> str) {
	    std::cerr << "done! len=" << str->length() << "\n";
	    std::cerr << "refcnt " << str.use_count() << "\n";
	});
    io_service.run();

    std::cerr << "run returns\n";
    
    return 0;
}

#endif
