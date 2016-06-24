/**
 * @file   mw/gdb/parsers/break_point.hpp
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
#ifndef MW_GDB_PARSERS_BREAK_POINT_HPP_
#define MW_GDB_PARSERS_BREAK_POINT_HPP_

#include <cstdint>
#include <boost/variant.hpp>
#include <mw/gdb/parsers/config.hpp>
#include <mw/gdb/parsers/location.hpp>

namespace mw
{
namespace gdb
{

struct bp_mult_loc
{
    std::string name;
    int num;
};

struct bp_decl
{
    int cnt;
    std::uint64_t ptr;
    boost::variant<location, bp_mult_loc> locs;
};



}
}

BOOST_FUSION_ADAPT_STRUCT(
    mw::gdb::bp_mult_loc,
    (std::string, name),
    (int, num)
);

BOOST_FUSION_ADAPT_STRUCT(
    mw::gdb::bp_decl,
    (int, cnt),
    (std::uint64_t, ptr),
    (decltype(mw::gdb::bp_decl::locs), locs)
);

namespace mw
{
namespace gdb
{
namespace parsers
{


x3::rule<class bp_multiple_loc, mw::gdb::bp_mult_loc> bp_multiple_loc;

auto bp_multiple_loc_def = x3::lexeme[+(!(x3::space | '.') >> x3::char_ ) ] >> x3::lit('.') >>
                       '(' >> x3::int_ >> "locations" >> x3::lit(')');

BOOST_SPIRIT_DEFINE(bp_multiple_loc);

x3::rule<class bp_decl_, mw::gdb::bp_decl> bp_decl;

auto bp_decl_def = "Breakpoint" >> x3::int_ >> "at" >> x3::lexeme["0x" >> x3::hex] >>
                   ':' >> (loc | bp_multiple_loc);

BOOST_SPIRIT_DEFINE(bp_decl);


}

}
}

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_decl,
    "Breakpoint 1 at 0x401679: file src\\..\\test\\target.cpp, line 15.",
    mw::gdb::bp_decl,
    ((attr.cnt,1), (attr.ptr, 0x401679),
     (boost::get<mw::gdb::location>(attr.locs).file, "src\\..\\test\\target.cpp")
     (boost::get<mw::gdb::location>(attr.locs).line, 15)));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_decl,
    "Breakpoint 3 at 0x401684: test2. (2 locations)",
    mw::gdb::bp_decl,
    ((attr.cnt,3), (attr.ptr, 0x401684),
     (boost::get<mw::gdb::bp_mult_loc>(attr.locs).name, "test2")
     (boost::get<mw::gdb::bp_mult_loc>(attr.locs).num, 2)));




#endif /* MW_GDB_PARSERS_INFO_HPP_ */
