/**
 * @file   /gdb-runner/test/parser.cpp
 * @date   23.06.2016
 * @author Klemens D. Morgenstern
 *
 * Published under [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html)
  <pre>
    /  /|  (  )   |  |  /
   /| / |   \/    | /| /
  / |/  |  / \    |/ |/
 /  /   | (   \   /  |
               )
 </pre>
 */

#include <boost/asio/spawn.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/facilities/overload.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <string>

#define TEST_2(Rule, String)                                          \
BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(mw_gdb_test_, __LINE__), *boost::unit_test::timeout(2))\
{                                                                     \
    std::string data = String;                                        \
                                                                      \
    auto itr = data.cbegin();                                         \
                                                                      \
    BOOST_CHECK(boost::spirit::x3::phrase_parse(itr, data.cend(), Rule, boost::spirit::x3::space)); \
    BOOST_CHECK(itr == data.cend()); \
}

#define TEST_3(Rule, String, Attr)                                          \
BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(mw_gdb_test_, __LINE__), *boost::unit_test::timeout(2))\
{                                                                           \
    std::string data = String;                                              \
                                                                            \
    auto itr = data.cbegin();                                               \
                                                                            \
    Attr attr;                                                              \
                                                                            \
    BOOST_CHECK(boost::spirit::x3::phrase_parse(itr, data.cend(), Rule, boost::spirit::x3::space, attr)); \
    BOOST_CHECK(itr == data.cend()); \
}

#define CHECK_EQUAL(lhs, rhs) BOOST_CHECK_EQUAL ( lhs , rhs );
#define CHECK_MACRO(r, data, elem) CHECK_EQUAL elem


#define TEST_4(Rule, String, Attr, Comparisons)                             \
BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(mw_gdb_test_, __LINE__), *boost::unit_test::timeout(2))\
{                                                                           \
    std::string data = String;                                              \
    auto itr = data.cbegin();                                               \
    Attr attr;                                                              \
                                                                            \
    BOOST_CHECK(boost::spirit::x3::phrase_parse(itr, data.cend(), Rule, boost::spirit::x3::space, attr)); \
    BOOST_CHECK(itr == data.cend()); \
    BOOST_PP_LIST_FOR_EACH ( CHECK_MACRO , _ , BOOST_PP_VARIADIC_TO_LIST Comparisons); \
}

#define MW_GDB_TEST_PARSER(args...) BOOST_PP_OVERLOAD(TEST_, args)(args)

#define INCLUDE() <mw/gdb/parsers/MW_GDB_HEADER>

#include INCLUDE()

BOOST_AUTO_TEST_CASE(dummy) {}
