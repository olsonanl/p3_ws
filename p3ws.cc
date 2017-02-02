#include <iostream>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/optional/optional_io.hpp>

#include <unistd.h>

#include "kserver.h"
#include "jsonrpc_handler.h"
#include "ws.h"
#include "ws_client.h"
#include "curl_aio.h"
#include "auth_mgr.h"

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
    std::string mongo_host;
    int mongo_port;
    std::string mongo_db;
    
    desc.add_options()
	("help,h", "show this help message")
	("listen-port-file", po::value<std::string>(&listen_port_file)->default_value("/dev/null"), "save the listen port to this file")
	("listen-port,l", po::value<std::string>(&listen_port)->required(), "port to listen on. 0 means to choose a random port")
	("mongo-host", po::value<std::string>(&mongo_host)->default_value("localhost"), "mongo hostname")
	("mongo-port", po::value<int>(&mongo_port)->default_value(27017), "mongo port")
	("mongo-database", po::value<std::string>(&mongo_db), "mongo dbname")
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

    if (mongo_db.empty())
    {
	std::cerr << "No mongo db specified\n";
	exit(0);
    }

    Workspace ws(mongo_host, mongo_port, mongo_db);

    std::string tstring(R"(un=olson@patricbrc.org|tokenid=63cbfdfb-a28a-4135-b24d-4e1217319c54|expiry=1515696940|client_id=olson@patricbrc.org|token_type=Bearer|realm=patricbrc.org|SigningSubject=https://user.patricbrc.org/public_key|sig=12096102320ee7354df5242eccb20abae85c553710ac375d09efa11839e470909e4c940b297c6e8ceae5d13f75e6b913cc87a61bab0639b7d64e70b84737ddbf977d6687d80f16ec502989fdf0531f213dd3eb6afc9d60eaa8bb366fbdc0a0c565eae8d0d01c75d0ab22568b18cf485a45e2d41d82fbe3498b8c709cd0933012)");
    std::string tstring2 = "un=olson|tokenid=0AD14BAA-E984-11E6-AAE0-F608692E0674|expiry=1517602400|client_id=olson|token_type=Bearer|SigningSubject=http://rast.nmpdr.org/goauth/keys/E087E220-F8B1-11E3-9175-BD9D42A49C03|this_is_globus=globus_style_token|sig=280e60c203b15c9ac7d5eafa9a20a67a1994689d6947c5d552828fe383b327be766d0e135d7d8606761113a1b9020b8d1a8a8d16f448f5c8ffa409f4f8c58f11d0726d1b7aa4bf480ddadec4ced0097ace9b7a3a01e2821fb0af995a198946a0d94633d7035ac913c22a90e7e90210ca848e30740ecf94e352e15dfed3a5d6dd";
    
    libcurl_wrapper::CurlAIO::global_init();
    libcurl_wrapper::CurlAIO curl_aio(io_service);

    AuthMgr auth_mgr(curl_aio, "cache");
    AuthToken at(auth_mgr.create_token(tstring2));
//    AuthToken at(auth_mgr.create_token(tstring));
    UserContext uc(at);

    if (auto itemx = ws.query_object(uc, WsPath("/olson/olson/x/y"), false))
    {
	WsItem &ent = itemx.get();
	std::cerr << "Found item: " <<  ent.name() << " " << ent.path() << " " << ent.object_owner() << " '" << ent.creation_time() << "' " << ent.type() << " " << ent.object_id() << "\n";
    }
    if (auto itemx = ws.query_object(uc, WsPath("/olson/olson/mf4"), false))
    {
	WsItem &ent = itemx.get();
	std::cerr << "Found item: " <<  ent.name() << " " << ent.path() << " " << ent.object_owner() << " '" << ent.creation_time() << "' " << ent.type() << " " << ent.object_id() << "\n";
    }
    else
	std::cerr << "didn't find mf4\n";

    auto res = ws.query_workspaces(uc);
    for (auto ent: res)
    {
	std::cerr << ent.name() << " " << ent.path() << " " << ent.object_owner() << " '" << ent.creation_time() << "' " << ent.type() << " " << ent.object_id() << "\n";
    }

    /*
      WorkspaceClient wsclient(ws);
      JsonRpcHandler jhandler;
      jhandler.register_module("Workspace", std::bind(&WorkspaceClient::handle_call, &wsclient, std::placeholders::_1, std::placeholders::_2));
      kserver->register_path("POST", "/api", std::bind(&JsonRpcHandler::handle_request, &jhandler, std::placeholders::_1));
      kserver->register_path("GET", "/quit", [&io_service](RequestPtr req) {
      std::cerr << "stopping on quit request\n";
      io_service.stop();
      });
      
      kserver->startup();
    */


    #ifdef GPROFILER
    std::cout << "profiler enable\n";
    ProfilerStart("prof.out");
    #endif
//    io_service.run();

    #ifdef GPROFILER
    ProfilerStop();
    std::cout << "profiler disable\n";
    #endif

    return 0;
}
