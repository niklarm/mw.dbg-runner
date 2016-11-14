/**
 * @file   mw/gdb/parsers/info.hpp
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
#ifndef MW_GDB_PARSERS_INFO_HPP_
#define MW_GDB_PARSERS_INFO_HPP_

#include <mw/gdb/parsers/config.hpp>

namespace mw
{
namespace gdb
{

struct info
{
    std::string toolset;
    std::string version;
    std::string config;
};

}
}

namespace mw
{
namespace gdb
{
namespace parsers
{

static x3::rule<class info_, mw::gdb::info> info;

static auto info_def =
        "GNU" >> x3::lit("gdb") >> x3::lexeme['(' >> +(!x3::lit(')') >> x3::char_ )  >> ')']   [set_member(&info::toolset)]
                                              >> x3::lexeme[+(!x3::space >> x3::char_)][set_member(&info::version)] >>
        (*x3::omit[!("This" >> x3::lit("GDB") >> "was" >> x3::lit("configured")) >> x3::char_])
        >> ("This" >> x3::lit("GDB") >> "was" >> x3::lit("configured") >> "as" >>
                '"' >> (+x3::no_skip[!x3::lit('"') >> x3::char_])[set_member(&info::config)] >> '"')
        >> +x3::omit[!x3::lit("(gdb)") >> x3::char_] >> "(gdb)";

BOOST_SPIRIT_DEFINE(info);

}
}
}

MW_GDB_TEST_PARSER(mw::gdb::parsers::info,
    "GNU gdb (GDB) 7.10.1\n"
    "Copyright (C) 2015 Free Software Foundation, Inc.\n"
    "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.  Type \"show copying\"\n"
    "and \"show warranty\" for details.\n"
    "This GDB was configured as \"x86_64-w64-mingw32\".\n"
    "Type \"show configuration\" for configuration details.\n"
    "For bug reporting instructions, please see:\n"
    "<http://www.gnu.org/software/gdb/bugs/>.\n"
    "Find the GDB manual and other documentation resources online at:\n"
    "<http://www.gnu.org/software/gdb/documentation/>.\n"
    "For help, type \"help\".\n"
    "Type \"apropos word\" to search for commands related to \"word\".\n"
    "(gdb)",
    mw::gdb::info,
    ((attr.toolset,"GDB"), (attr.version, "7.10.1"), (attr.config, "x86_64-w64-mingw32")))

MW_GDB_TEST_PARSER(mw::gdb::parsers::info,
    "GNU gdb (GNU Tools for ARM Embedded Processors) 7.10.1.20151217-cvs\n"
    "Copyright (C) 2015 Free Software Foundation, Inc.\n"
    "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.  Type \"show copying\"\n"
    "and \"show warranty\" for details.\n"
    "This GDB was configured as \"--host=i686-w64-mingw32 --target=arm-none-eabi\".\n"
    "Type \"show configuration\" for configuration details.\n"
    "For bug reporting instructions, please see:\n"
    "<http://www.gnu.org/software/gdb/bugs/>.\n"
    "Find the GDB manual and other documentation resources online at:\n"
    "<http://www.gnu.org/software/gdb/documentation/>.\n"
    "For help, type \"help\".\n"
    "Type \"apropos word\" to search for commands related to \"word\".\n"
    "(gdb)",
    mw::gdb::info,
    ((attr.toolset,"GNU Tools for ARM Embedded Processors"),
     (attr.version, "7.10.1.20151217-cvs"),
     (attr.config, "--host=i686-w64-mingw32 --target=arm-none-eabi")))




#endif /* MW_GDB_PARSERS_INFO_HPP_ */
