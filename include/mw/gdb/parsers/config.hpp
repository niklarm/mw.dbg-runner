/**
 * @file   mw/gdb/parsers/config.hpp
 * @date   23.06.2016
 * @author Klemens D. Morgenstern
 *
 * Published under [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html)
 *
 <pre>
    /  /|  (  )   |  |  /
   /| / |   \/    | /| /
  / |/  |  / \    |/ |/
 /  /   | (   \   /  |
               )
 </pre>
 */

#ifndef MW_GDB_PARSER_CONFIG_HPP_
#define MW_GDB_PARSER_CONFIG_HPP_

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <iterator>
#include <boost/spirit/home/support/multi_pass.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


#if !defined ( MW_GDB_TEST_PARSER )
#define MW_GDB_TEST_PARSER(...)
#endif

namespace mw
{
namespace gdb
{
namespace parsers
{

namespace x3 = boost::spirit::x3;
using buf_iterator  = std::istreambuf_iterator<char>;
using iterator      = boost::spirit::multi_pass<buf_iterator>;

using x3::string;
using x3::char_;
using x3::uint_;
using x3::eol;
using x3::eoi;
using x3::eps;
using x3::no_skip;
using x3::skip;
using x3::space;
using x3::_val;
using x3::_attr;
using x3::lit;
using x3::lexeme;
using x3::omit;
using x3::raw;
using boost::fusion::at_c;

///Utility, to declare a c++14 lambda directly in a rule
auto static l = [](auto l){return l;};

}
}
}



#endif /* MW_TEST_PARSER_CONFIG_HPP_ */
