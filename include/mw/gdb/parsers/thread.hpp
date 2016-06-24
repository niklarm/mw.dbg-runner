/**
 * @file   mw/gdb/parsers/thread.hpp
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
#ifndef MW_GDB_PARSERS_THREAD_HPP_
#define MW_GDB_PARSERS_THREAD_HPP_

#include <mw/gdb/parsers/config.hpp>

namespace mw
{
namespace gdb
{

struct thread_info
{
    int           proc;
    std::uint64_t thr;
};

struct exit_thread : thread_info
{
    int code;
};

struct exit_proc
{
    int idx;
    int proc;
    int code;

};

}
}

BOOST_FUSION_ADAPT_STRUCT(
    mw::gdb::thread_info,
    (int, proc),
    (std::uint64_t, thr),
);

BOOST_FUSION_ADAPT_STRUCT(
    mw::gdb::exit_thread,
    (int, proc),
    (std::uint64_t, thr),
    (int, code)
);

BOOST_FUSION_ADAPT_STRUCT(
    mw::gdb::exit_proc,
    (int, idx),
    (int, proc),
    (int, code)
);


namespace mw
{
namespace gdb
{
namespace parsers
{

x3::rule<class start_prog, std::string> start_prog;
auto start_prog_def = "Starting" >> x3::lit("program:") >> x3::lexeme[+(!x3::space >> x3::char_)];

BOOST_SPIRIT_DEFINE(start_prog);

x3::rule<class switch_thread, mw::gdb::thread_info> switch_thread;
auto switch_thread_def = '[' >> x3::lit("Switching") >> "to" >> x3::lit("Thread") >> x3::int_ >> "."
                         >> x3::lit("0x")  >> x3::hex >> ']';

BOOST_SPIRIT_DEFINE(switch_thread);

x3::rule<class exit_thread_, mw::gdb::exit_thread> exit_thread;
auto exit_thread_def = '[' >> x3::lit("Thread") >> x3::int_ >> '.' >> x3::lexeme[x3::lit("0x") >> x3::hex] >>
                       "exited" >> x3::lit("with") >> "code" >> x3::int_ >> ']';

BOOST_SPIRIT_DEFINE(exit_thread);

x3::rule<class start_thread, mw::gdb::thread_info> start_thread;
auto start_thread_def  = ('[' >> x3::lit("New") >> "Thread" >> x3::int_ >> "."
                          >> x3::lit("0x")  >> x3::hex >> ']');

BOOST_SPIRIT_DEFINE(start_thread);


//[Inferior 1 (process 5304) exited with code 052]

x3::rule<class proc_end, mw::gdb::exit_proc> exit_proc;
auto exit_proc_def = '[' >> x3::lit("Inferior") >> x3::int_ >> '(' >> x3::lit("process") >> x3::int_ >> ')' >>
                     x3::lit("exited") >> "with" >> x3::lit("code") >> x3::oct >> ']';

BOOST_SPIRIT_DEFINE(exit_proc);

}
}
}

MW_GDB_TEST_PARSER(mw::gdb::parsers::start_prog,
    "Starting program: F:\\mwspace\\gdb-runner\\bin\\target.exe",
    std::string,
    ((attr, "F:\\mwspace\\gdb-runner\\bin\\target.exe")))


MW_GDB_TEST_PARSER(mw::gdb::parsers::switch_thread,
    "[Switching to Thread 4740.0xf4]",
    mw::gdb::thread_info,
    ((attr.proc, 4740), (attr.thr, 0xf4)))

MW_GDB_TEST_PARSER(mw::gdb::parsers::exit_thread,
    "[Thread 4740.0x2158 exited with code 42]",
    mw::gdb::exit_thread,
    ((attr.proc, 4740), (attr.thr, 0x2158), (attr.code, 42)))

MW_GDB_TEST_PARSER(mw::gdb::parsers::start_thread,
    "[New Thread 5304.0x4ec]",
    mw::gdb::thread_info,
    ((attr.proc, 5304), (attr.thr, 0x4EC)))

MW_GDB_TEST_PARSER(mw::gdb::parsers::exit_proc,
    "[Inferior 1 (process 5304) exited with code 052]",
    mw::gdb::exit_proc,
    ((attr.idx, 1), (attr.proc, 5304), (attr.code, 42)))





#endif /* MW_GDB_PARSERS_INFO_HPP_ */
