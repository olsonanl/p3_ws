#define BOOST_TEST_MODULE get tests
#include <boost/test/included/unit_test.hpp>

#include <boost/optional/optional_io.hpp>
#include "ws_path.h"
#include "ws.h"
#include "curl_aio.h"
#include "auth_token.h"
#include "auth_mgr.h"

class Fixture
{
public:
    Fixture() : 
	ws("127.0.0.1", 27017, "WorkspaceBob"),
	curl_aio(io_service),
//	tstring(R"(un=olson@patricbrc.org|tokenid=63cbfdfb-a28a-4135-b24d-4e1217319c54|expiry=1515696940|client_id=olson@patricbrc.org|token_type=Bearer|realm=patricbrc.org|SigningSubject=https://user.patricbrc.org/public_key|sig=12096102320ee7354df5242eccb20abae85c553710ac375d09efa11839e470909e4c940b297c6e8ceae5d13f75e6b913cc87a61bab0639b7d64e70b84737ddbf977d6687d80f16ec502989fdf0531f213dd3eb6afc9d60eaa8bb366fbdc0a0c565eae8d0d01c75d0ab22568b18cf485a45e2d41d82fbe3498b8c709cd0933012)"),
	tstring("un=olson|tokenid=0AD14BAA-E984-11E6-AAE0-F608692E0674|expiry=1517602400|client_id=olson|token_type=Bearer|SigningSubject=http://rast.nmpdr.org/goauth/keys/E087E220-F8B1-11E3-9175-BD9D42A49C03|this_is_globus=globus_style_token|sig=280e60c203b15c9ac7d5eafa9a20a67a1994689d6947c5d552828fe383b327be766d0e135d7d8606761113a1b9020b8d1a8a8d16f448f5c8ffa409f4f8c58f11d0726d1b7aa4bf480ddadec4ced0097ace9b7a3a01e2821fb0af995a198946a0d94633d7035ac913c22a90e7e90210ca848e30740ecf94e352e15dfed3a5d6dd"),
	auth_mgr(curl_aio, "cache"),
	at(auth_mgr.create_token(tstring)),
	uc(at)
	{
	    libcurl_wrapper::CurlAIO::global_init();
	    std::cerr << "Initialized\n";
	    instance_ = this;
	}
    ~Fixture() {
	std::cerr << "Finished\n";
    }
    static Fixture *instance() { return instance_; }
    static Fixture *instance_;
    boost::asio::io_service io_service;
    Workspace ws;
    libcurl_wrapper::CurlAIO curl_aio;

	   std::string tstring;
    AuthMgr auth_mgr;
    AuthToken at;
    UserContext uc;


};

Fixture *Fixture::instance_;

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(t1)
{
    Fixture *f = Fixture::instance();
    auto item = f->ws.query_object(f->uc, WsPath("/olson/olson/x/y"), false);
    BOOST_TEST((item ? true : false));
}
BOOST_AUTO_TEST_CASE(t2)
{
    Fixture *f = Fixture::instance();
    std::cerr << "test2" << f->uc << "\n";
}

//BOOST_AUTO_TEST_SUITE_END()
