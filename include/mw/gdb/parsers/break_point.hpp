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
#include <vector>
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

struct var
{
    std::string value;
    std::string policy;
    std::string cstring;
};

struct arg : var
{
    std::string id;
};

struct bp_stop
{
    int index;
    std::string name;
    std::vector<arg> args;
    location loc;
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


BOOST_FUSION_ADAPT_STRUCT
(
    mw::gdb::var,
    (std::string, value)
    (std::string, policy)
    (std::string, cstring)
);

BOOST_FUSION_ADAPT_STRUCT
(
    mw::gdb::arg,
    (std::string, id)
    (std::string, value)
    (std::string, policy)
    (std::string, cstring)
);

BOOST_FUSION_ADAPT_STRUCT
(
    mw::gdb::bp_stop,
    (int, index),
    (std::string, name),
    (std::vector<mw::gdb::arg>, args),
    (mw::gdb::location, loc)
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




x3::rule<class bp_arg_list_step, arg> bp_arg_list_step;

auto bp_arg_list_step_def = +(!(x3::lit('=') | ',' | ')') >> x3::char_) >> '=' >>
                    x3::lexeme[+(!(x3::space | ',' | ')') >> x3::char_) ] >>
                    -x3::lexeme['<' >> *(!x3::lit('>') >> x3::char_) >> '>' ] >>
                    -x3::lexeme['"' >> *(!x3::lit('"') >> x3::char_) >> '"' ];

BOOST_SPIRIT_DEFINE(bp_arg_list_step);

x3::rule<class bp_arg_list, std::vector<arg>> bp_arg_list;

auto bp_arg_list_def = '(' >> -(bp_arg_list_step % ',' ) >> ')'; //)[set_bp_arg_list];

BOOST_SPIRIT_DEFINE(bp_arg_list);


x3::rule<class bp_stop, mw::gdb::bp_stop> bp_stop;

auto bp_stop_def = ("Breakpoint" >> x3::int_ >> "," >> x3::lexeme[+(!x3::space >> x3::char_)] >>
               bp_arg_list >>
               x3::lit("at") >>
               x3::lexeme[+(!x3::space >> x3::char_) >> ':' >> x3::int_]
               >> *(!x3::lit("(gdb)") >> x3::char_)) >> "(gdb)";

BOOST_SPIRIT_DEFINE(bp_stop);

x3::rule<class var_, mw::gdb::var> var;

auto var_def = x3::omit[x3::lexeme['$' >> x3::int_]] >> '=' >>
                x3::lexeme[+(!x3::space >> x3::char_)] >>
                -x3::lexeme['<' >> *(!x3::lit('>') >> x3::char_) >> '>' ] >>
                -x3::lexeme['"' >> *(!x3::lit('"') >> x3::char_) >> '"' ] >> "(gdb)";

BOOST_SPIRIT_DEFINE(var);

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


MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list_step, "c=0x0",
                   mw::gdb::arg,
                   ((attr.id, "c"),
                    (attr.value, "0x0"),
                    (attr.policy, ""),
                    (attr.cstring, "")
                    ));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list_step, "c=0x496048 <__gnu_cxx::__default_lock_policy+4> \"Thingy\"",
                   mw::gdb::arg,
                   ((attr.id, "c"),
                    (attr.value, "0x496048"),
                    (attr.policy, "__gnu_cxx::__default_lock_policy+4"),
                    (attr.cstring, "Thingy")
                    ));



MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list, "()",
                   std::vector<mw::gdb::arg>,
                   ((attr.size(), 0)));


MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list, "(c=0x0)",
                   std::vector<mw::gdb::arg>,
                   ((attr.size(), 1),
                    (attr.at(0).id, "c"),
                    (attr.at(0).value, "0x0")));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list, "(c=0x496048 <__gnu_cxx::__default_lock_policy+4> \"Thingy\")",
                   std::vector<mw::gdb::arg>,
                   ((attr.size(), 1),
                   (attr.at(0).id, "c"),
                   (attr.at(0).value, "0x496048"),
                   (attr.at(0).policy, "__gnu_cxx::__default_lock_policy+4"),
                   (attr.at(0).cstring, "Thingy")));

MW_GDB_TEST_PARSER(mw::gdb::parsers::var, "$3 = 0x4a5048 <__gnu_cxx::__default_lock_policy+4> \"Thingy\" (gdb)",
                   mw::gdb::var,
                   ((attr.value, "0x4a5048"),
                    (attr.policy, "__gnu_cxx::__default_lock_policy+4"),
                    (attr.cstring, "Thingy")));




#endif /* MW_GDB_PARSERS_INFO_HPP_ */
