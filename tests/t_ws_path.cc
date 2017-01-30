#define BOOST_TEST_MODULE path tests
#include <boost/test/included/unit_test.hpp>

#include "ws_path.h"

BOOST_AUTO_TEST_CASE(empty)
{
    BOOST_CHECK_THROW(WsPath(""), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(user_only)
{
    WsPath p("/user");
    BOOST_TEST(p.user() == "user");
    BOOST_TEST(p.workspace() == "");
    BOOST_TEST(p.delimited_path() == "");
    BOOST_TEST(p.name() == "");
}

BOOST_AUTO_TEST_CASE(user_ws_only)
{
    WsPath p("/user/ws");
    BOOST_TEST(p.user() == "user");
    BOOST_TEST(p.workspace() == "ws");
    BOOST_TEST(p.delimited_path() == "");
    BOOST_TEST(p.name() == "");
}

BOOST_AUTO_TEST_CASE(dir1)
{
    WsPath p("/user/ws/foo");
    BOOST_TEST(p.user() == "user");
    BOOST_TEST(p.workspace() == "ws");
    BOOST_TEST(p.delimited_path() == "");
    BOOST_TEST(p.name() == "foo");
}


BOOST_AUTO_TEST_CASE(dir2)
{
    WsPath p("/user/ws/foo/bar");
    BOOST_TEST(p.user() == "user");
    BOOST_TEST(p.workspace() == "ws");
    BOOST_TEST(p.delimited_path() == "foo");
    BOOST_TEST(p.name() == "bar");
}


BOOST_AUTO_TEST_CASE(dir3)
{
    WsPath p("/user/ws/foo/bar/");
    BOOST_TEST(p.user() == "user");
    BOOST_TEST(p.workspace() == "ws");
    BOOST_TEST(p.delimited_path() == "foo");
    BOOST_TEST(p.name() == "bar");
}


