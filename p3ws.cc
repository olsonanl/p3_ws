#include <iostream>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <unistd.h>

#include "kserver.h"

#ifdef BLCR_SUPPORT
#include "libcr.h"
#endif

#ifdef GPROFILER
#include <gperftools/profiler.h>
#endif

#define DEFINE_GLOBALS 1
#include "global.h"

using namespace boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    std::ostringstream x;
    x << "Usage: " << argv[0] << " [options] listen-port\nAllowed options";
    po::options_description desc(x.str());

    std::string listen_port;
    std::string listen_port_file;
    bool daemonize;
    std::string pid_file;
    
    desc.add_options()
	("help,h", "show this help message")
	("listen-port-file", po::value<std::string>(&listen_port_file)->default_value("/dev/null"), "save the listen port to this file")
	("listen-port,l", po::value<std::string>(&listen_port)->required(), "port to listen on. 0 means to choose a random port")
	("debug-http", po::bool_switch(), "Debug HTTP protocol")
	("daemonize", po::bool_switch(&daemonize), "Run the service in the background")
	("pid-file", po::value<std::string>(&pid_file), "Write the process id to this file")
	;
    po::positional_options_description pd;
    pd.add("listen-port", 1);

    po::variables_map vm;
    g_parameters = &vm;
    po::store(po::command_line_parser(argc, argv).
	      options(desc).positional(pd).run(), vm);

    if (vm.count("help"))
    {
	std::cout << desc << "\n";
	return 1;
    }

    try {
	po::notify(vm);
    } catch (po::required_option &e)
    {
	std::cerr << "Invalid command line: " << e.what() << "\n";
	std::cerr << desc << "\n";
	exit(1);
    }

    #ifdef BLCR_SUPPORT
    // g_cr_client_id = cr_init();
    #endif

    if (daemonize)
    {
	pid_t child = fork();
	if (child < 0)
	{
	    std::cerr << "fork failed: " << strerror(errno) << "\n";
	    exit(1);
	}
	if (child > 0)
	{
	    if (!pid_file.empty())
	    {
		std::ofstream pf(pid_file);
		pf << child << "\n";
		pf.close();
	    }
	    exit(0);
	}
	pid_t sid = setsid();
	if (sid < 0)
	{
	    std::cerr << "setsid failed: " << strerror(errno) << "\n";
	    exit(1);
	}
    }
    else if (!pid_file.empty())
    {
	std::ofstream pf(pid_file);
	pf << getpid() << "\n";
	pf.close();
    }

    boost::asio::io_service io_service;

    std::shared_ptr<RequestServer> kserver = std::make_shared<RequestServer>(io_service, listen_port, listen_port_file);

    kserver->startup();

    #ifdef GPROFILER
    std::cout << "profiler enable\n";
    ProfilerStart("prof.out");
    #endif
    io_service.run();

    #ifdef GPROFILER
    ProfilerStop();
    std::cout << "profiler disable\n";
    #endif

    return 0;
}
