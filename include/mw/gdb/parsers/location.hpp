/**
 * @file   mw/gdb/parsers/location.hpp
 * @date   24.06.2016
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
#ifndef MW_GDB_PARSERS_LOCATION_HPP_
#define MW_GDB_PARSERS_LOCATION_HPP_

#include <mw/gdb/parsers/config.hpp>
#include <mw/gdb/location.hpp>
#include <string>


BOOST_FUSION_ADAPT_STRUCT(
    mw::gdb::location,
    (std::string, file),
    (int, line)
);

namespace mw
{
namespace gdb
{
namespace parsers
{

x3::rule<class loc_short, mw::gdb::location> loc_short;

auto loc_short_def = x3::lexeme[+(!(x3::space | ':' ) >> x3::char_)] >> ":" >> x3::int_;

BOOST_SPIRIT_DEFINE(loc_short);

x3::rule<class loc, mw::gdb::location> loc;

auto loc_def = "file" >> x3::lexeme[+(!x3::lit(',') >> x3::char_)] >> "," >> x3::lit("line") >> x3::int_ >> '.';

BOOST_SPIRIT_DEFINE(loc);

}
}
}

MW_GDB_TEST_PARSER(mw::gdb::parsers::loc_short, "src\\..\\test\\target.cpp:15",
    mw::gdb::location,
    ((attr.file, "src\\..\\test\\target.cpp"), (attr.line, 15)))

MW_GDB_TEST_PARSER(mw::gdb::parsers::loc, "file test\\target.cpp, line 31.",
    mw::gdb::location,
    ((attr.file, "test\\target.cpp"), (attr.line, 31)))

#endif /* MW_GDB_PARSERS_LOCATION_HPP_ */
