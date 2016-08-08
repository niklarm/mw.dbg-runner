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

#include <mw/gdb/frame.hpp>
#include <cstdint>
#include <vector>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
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
    mw::gdb::cstring_t,
    (std::string, value)
    (bool, ellipsis)
);

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
    (boost::optional<std::uint64_t>, ref)
    (std::string, value)
    (std::string, policy)
    (mw::gdb::cstring_t, cstring)
);

BOOST_FUSION_ADAPT_STRUCT
(
    mw::gdb::arg,
    (std::string, id)
    (boost::optional<std::uint64_t>, ref)
    (std::string, value)
    (std::string, policy)
    (mw::gdb::cstring_t, cstring)
);

BOOST_FUSION_ADAPT_STRUCT
(
    mw::gdb::bp_stop,
    (int, index),
    (std::string, name),
    (std::vector<mw::gdb::arg>, args),
    (mw::gdb::location, loc)
);

BOOST_FUSION_ADAPT_STRUCT
(
    mw::gdb::backtrace_elem,
    (int, cnt)
    (boost::optional<std::uint64_t>, call_site)
    (std::string, func)
    (std::string, args)
    (mw::gdb::location, loc)
);


namespace mw
{
namespace gdb
{
namespace parsers
{

static auto ellipsis_enable  = [](auto & ctx){x3::_val(ctx) = true;};
static auto ellipsis_disable = [](auto & ctx){x3::_val(ctx) = false;};

static x3::rule<class ellipsis, bool> ellipsis;
static auto ellipsis_def = x3::lit("...")[ellipsis_enable] | x3::eps[ellipsis_disable];

BOOST_SPIRIT_DEFINE(ellipsis);


static x3::rule<class quoted_string, std::string> quoted_string;
static auto quoted_string_def =
        x3::lexeme['"' >> *(x3::string("\\\"") | (!x3::lit('"') >> x3::char_)) >> '"'];

BOOST_SPIRIT_DEFINE(quoted_string);

static x3::rule<class cstring, mw::gdb::cstring_t> cstring;
static auto cstring_def =
        quoted_string >> ellipsis;

BOOST_SPIRIT_DEFINE(cstring);

static x3::rule<class bp_multiple_loc, mw::gdb::bp_mult_loc> bp_multiple_loc;

static auto bp_multiple_loc_def = x3::lexeme[+(!(x3::space | '.') >> x3::char_ ) ] >> x3::lit('.') >>
                       '(' >> x3::int_ >> "locations" >> x3::lit(')');

BOOST_SPIRIT_DEFINE(bp_multiple_loc);

static x3::rule<class bp_decl_, mw::gdb::bp_decl> bp_decl;

static auto bp_decl_def = "Breakpoint" >> x3::int_ >> "at" >> x3::lexeme["0x" >> x3::hex] >>
                   ':' >> (loc | bp_multiple_loc);

BOOST_SPIRIT_DEFINE(bp_decl);

//arg0=arg0@entry=0x0
static x3::rule<class bp_arg_name, std::string> bp_arg_name;

static auto bp_arg_name_set = [](auto & ctx){x3::_val(ctx)  = x3::_attr(ctx);};
static auto bp_arg_name_cmp = [](auto & ctx){x3::_pass(ctx) = (x3::_attr(ctx) == x3::_val(ctx));};

static auto bp_arg_name_def =
        (+(!(x3::lit('=') | ',' | ')') >> x3::char_))[bp_arg_name_set] >>
        -('=' >> (+(!(x3::lit('=') | ',' | '@') >> x3::char_))[bp_arg_name_cmp] >> '@' >> x3::lit("entry"));

BOOST_SPIRIT_DEFINE(bp_arg_name);


static x3::rule<class bp_arg_list_step, arg> bp_arg_list_step;

static auto bp_arg_list_step_def = bp_arg_name >> '=' >>
                    -("@0x" >> x3::hex >> ":") >>
                    (quoted_string | x3::lexeme[+(!(x3::space | ',' | ')') >> x3::char_) ] ) >>
                    -x3::lexeme['<' >> *(!x3::lit('>') >> x3::char_) >> '>' ] >>
                    -x3::lexeme[cstring];

BOOST_SPIRIT_DEFINE(bp_arg_list_step);

static x3::rule<class bp_arg_list, std::vector<arg>> bp_arg_list;

static auto bp_arg_list_def = '(' >> -(bp_arg_list_step % ',' ) >> ')'; //)[set_bp_arg_list];

BOOST_SPIRIT_DEFINE(bp_arg_list);

static x3::rule<class func_name, std::string> func_name;

static auto func_name_def = x3::eps;

BOOST_SPIRIT_DEFINE(func_name);

static x3::rule<class bp_stop_, mw::gdb::bp_stop> bp_stop;

static auto bp_stop_def = ("Breakpoint" >> x3::int_ >> "," >> x3::lexeme[+(!x3::space >> x3::char_)] >>
               bp_arg_list >>
               x3::lit("at") >>
               loc_short //x3::lexeme[+(!(x3::space | ':') >> x3::char_) >> ':' >> x3::int_]
               >> x3::omit[*(!x3::lit("(gdb)") >> x3::char_)]) >> "(gdb)";

BOOST_SPIRIT_DEFINE(bp_stop);


static x3::rule<class round_brace, std::string> round_brace;

static auto round_brace_def = '(' >> *((!(x3::lit('(') | ')') >> x3::char_) | round_brace ) >> ')';

BOOST_SPIRIT_DEFINE(round_brace);

static x3::rule<class strict_var, mw::gdb::var> strict_var;

static auto strict_var_def = x3::omit[x3::lexeme['$' >> x3::int_]] >> '=' >>
                -(-x3::omit[round_brace] >> "@0x" >> x3::hex >> ':') >>
                x3::lexeme[+(!x3::space >> x3::char_)] >>
                -x3::lexeme['<' >> *(!x3::lit('>') >> x3::char_) >> '>'] >>
                -x3::lexeme[cstring] >>
                -x3::omit[x3::lexeme['\'' >> +(
                           x3::lit("\\\\") |  "\\'" | (!x3::lit('\'') >> x3::char_)) >> '\''] ];

BOOST_SPIRIT_DEFINE(strict_var);

static x3::rule<class relaxed_var, mw::gdb::var> relaxed_var;

static auto relaxed_var_value = [](auto & ctx)
        {
            auto r = x3::_attr(ctx);
            x3::_val(ctx).value.assign(r.begin(), r.end());
        };

static auto relaxed_var_def = x3::omit[x3::lexeme['$' >> x3::int_]] >> '=' >>
                       x3::raw[x3::lexeme[*(!(x3::lit("(gdb)") >> x3::char_))]][relaxed_var_value];

BOOST_SPIRIT_DEFINE(relaxed_var);

static x3::rule<class var_, mw::gdb::var> var;
static auto var_def = ( strict_var | relaxed_var );

BOOST_SPIRIT_DEFINE(var);

//#0  my_func (value=false, c1=0x405053 <std::piecewise_construct+19> "d", c2=0x40504b <std::piecewise_construct+11> "(char)2") at test.mwt:125
//#1  0x0000000000401638 in <lambda()>::operator()(void) const (__closure=0x62fe20) at test.mwt:1003
//#2  0x000000000040168c in __mw_execute<main(int, char**)::<lambda()> >(<lambda()>) (func=...) at test.mwt:131
//#3  0x000000000040166c in main (argc=1, argv=0xe64c10) at test.mwt:1003

static x3::rule<class backtrace_elem_, mw::gdb::backtrace_elem> backtrace_elem;

static auto backtrace_elem_def = '#' >> x3::int_ >> -(x3::lexeme["0x" >> x3::hex]  >> "in")
        >> *(!(round_brace >> "at" >> loc_short) >> x3::char_)
        >> round_brace >> "at" >> loc_short;


BOOST_SPIRIT_DEFINE (backtrace_elem);

static x3::rule<class backtrace, std::vector<mw::gdb::backtrace_elem>> backtrace;

static auto backtrace_def = *backtrace_elem;

BOOST_SPIRIT_DEFINE(backtrace);

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


MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list_step, "ref=@0x61fe4c: 0",
                   mw::gdb::arg,
                   ((attr.id, "ref"),
                    (*attr.ref, 0x61fe4c),
                    (attr.value, "0")
                    ));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list_step, "c=0x0",
                   mw::gdb::arg,
                   ((attr.id, "c"),
                    (attr.value, "0x0"),
                    (attr.policy, ""),
                    (attr.cstring.value, "")
                    ));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list_step, "c=0x496048 <__gnu_cxx::__default_lock_policy+4> \"Thingy\"",
                   mw::gdb::arg,
                   ((attr.id, "c"),
                    (attr.value, "0x496048"),
                    (attr.policy, "__gnu_cxx::__default_lock_policy+4"),
                    (attr.cstring.value, "Thingy")
                    ));



MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list, "()",
                   std::vector<mw::gdb::arg>,
                   ((attr.size(), 0)));


MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list, "(c=0x0)",
                   std::vector<mw::gdb::arg>,
                   ((attr.size(), 1),
                    (attr.at(0).id, "c"),
                    (attr.at(0).value, "0x0")));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list, "(st=\"original data\")",
                   std::vector<mw::gdb::arg>,
                   ((attr.size(), 1),
                    (attr.at(0).id, "st"),
                    (attr.at(0).value, "original data")));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_list, "(c=0x496048 <__gnu_cxx::__default_lock_policy+4> \"Thingy\")",
                   std::vector<mw::gdb::arg>,
                   ((attr.size(), 1),
                   (attr.at(0).id, "c"),
                   (attr.at(0).value, "0x496048"),
                   (attr.at(0).policy, "__gnu_cxx::__default_lock_policy+4"),
                   (attr.at(0).cstring.value, "Thingy")));

MW_GDB_TEST_PARSER(mw::gdb::parsers::var, "$3 = 0x4a5048 <__gnu_cxx::__default_lock_policy+4> \"Thingy\" (gdb)",
                   mw::gdb::var,
                   ((attr.value, "0x4a5048"),
                    (attr.policy, "__gnu_cxx::__default_lock_policy+4"),
                    (attr.cstring.value, "Thingy")));


MW_GDB_TEST_PARSER(mw::gdb::parsers::round_brace, "(int &)");

MW_GDB_TEST_PARSER(mw::gdb::parsers::var, "$1 = (int &) @0x61fe4c: 0 \n (gdb)",
                   mw::gdb::var,
                   ((attr.value, "0"),
                    (*attr.ref, 0x61fe4c)));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_stop,
        "Breakpoint 1, st (st=\"original data\") at src\\..\\test\\target.cpp:31\n"
        "31      }\n"
        "(gdb)");

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_stop,
        "Breakpoint 1, f (p=0x61fe40) at target.cpp:21\n"
        "21  }\n"
        "(gdb)");

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_stop,
        "Breakpoint 2, test_func (c=0x0, i=0) at src\\..\\test\\target.cpp:15\n"
        "15      }\n"
        "(gdb)",
        mw::gdb::bp_stop,
        ((attr.index, 2),
         (attr.name, "test_func"),
         (attr.args.size(), 2),
         (attr.loc.line, 15)));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_name,
        "arg0=arg0@entry",
        std::string,
        ((attr, "arg0")));

MW_GDB_TEST_PARSER(mw::gdb::parsers::bp_arg_name,
        "arg1",
        std::string,
        ((attr, "arg1")));

//MW_GDB_TEST_PARSER(mw::gdb::parsers::backtrace,
//        "#0  my_func (value=false, c1=0x405053 <std::piecewise_construct+19> \"d\", c2=0x40504b <std::piecewise_construct+11> \"(char)2\") at test.mwt:125\n"
//        "#1  0x0000000000401638 in <lambda()>::operator()(void) const (__closure=0x62fe20) at test.mwt:1003\n"
//        "#2  0x000000000040168c in __mw_execute<main(int, char**)::<lambda()> >(<lambda()>) (func=...) at test.mwt:131\n"
//        "#3  0x000000000040166c in main (argc=1, argv=0xe64c10) at test.mwt:1003\n",
//        std::vector<mw::gdb::backtrace_elem>,
//        ((attr.size(), 4),
//         (attr[0].cnt, 0),
//         (attr[2].loc.file, "test.mwt"),
//         (attr[2].loc.line, 131),
//         (attr[1].cnt, 1))
//)


#endif /* MW_GDB_PARSERS_INFO_HPP_ */
