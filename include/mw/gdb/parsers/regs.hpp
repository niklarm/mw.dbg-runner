/**
 * @file   mw/gdb/parsers/regs.hpp
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
#ifndef MW_GDB_PARSERS_REGS_HPP_
#define MW_GDB_PARSERS_REGS_HPP_

#include <mw/gdb/parsers/config.hpp>
#include <vector>

namespace mw
{
namespace gdb
{

struct reg
{
    std::string   id;
    std::uint64_t loc;
};

}
}

BOOST_FUSION_ADAPT_STRUCT(
    mw::gdb::reg,
    (std::string  , id),
    (std::uint64_t, loc));

namespace mw
{
namespace gdb
{
namespace parsers
{

x3::rule<class id, std::string> id;
auto id_def = x3::lexeme[x3::char_("A-Za-z") >> *x3::char_("A-Za-z0-9") ];

BOOST_SPIRIT_DEFINE(id);

x3::rule<class reg_pointy, std::string> reg_pointy;
auto reg_pointy_def = '<' >> *(!x3::lit('>') >> (reg_pointy | x3::char_ )) >> '>';

BOOST_SPIRIT_DEFINE(reg_pointy);

x3::rule<class reg_, mw::gdb::reg> reg;
auto reg_def = id >> ("0x") >> x3::hex >>
               x3::omit[x3::lexeme['[' >> *(!x3::lit(']') >> (x3::char_ | x3::space)) >> ']']
                      | x3::lexeme[+(!x3::space >> x3::char_)]] >>
               -x3::omit[reg_pointy];

BOOST_SPIRIT_DEFINE(reg);

x3::rule<class regs, std::vector<mw::gdb::reg>> regs;
auto regs_def = *reg >> "(gdb)";

BOOST_SPIRIT_DEFINE(regs);

}
}
}



MW_GDB_TEST_PARSER(mw::gdb::parsers::reg_pointy,
    "<st(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)+8>");

MW_GDB_TEST_PARSER(*mw::gdb::parsers::reg,
    "rax            0x6ffe10 7339536\n"
    "rbx            0x1      1\n"
    "rcx            0x6ffe10 7339536\n"
    "rdx            0x6ffe20 7339552\n"
    "rsi            0x25     37\n"
    "rdi            0x174b00 1526528\n"
    "rbp            0x6ffde0 0x6ffde0\n"
    "rsp            0x6ffde0 0x6ffde0\n"
    "r8             0x0      0\n"
    "r9             0x0      0\n"
    "r10            0x10     16\n"
    "r11            0x6ffe20 7339552\n"
    "r12            0x1      1\n"
    "r13            0x8      8\n"
    "r14            0x0      0\n"
    "r15            0x0      0\n"
    "rip            0x4016a1 0x4016a1 <st(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)+8>\n"
    "eflags         0x206    [ PF IF ]\n"
    "cs             0x33     51\n"
    "ss             0x2b     43\n"
    "ds             0x2b     43\n"
    "es             0x2b     43\n"
    "fs             0x53     83\n"
    "gs             0x2b     43\n"
    "(gdb)",
    std::vector<mw::gdb::reg>,
    ((attr.at(0).id, "rax"),  (attr.at(0).loc, 0x6ffe10),
     (attr.at(16).id, "rip"), (attr.at(16).loc, 0x4016a1),
     (attr.at(20).id, "ds"),  (attr.at(20).loc, 0x2b))
)


#endif /* MW_GDB_PARSERS_INFO_HPP_ */
