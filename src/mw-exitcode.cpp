/**
 * @file   mw-exitcode.cpp
 * @date   12.10.2016
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

#include <mw/gdb/break_point.hpp>
#include <mw/gdb/frame.hpp>

#include <vector>
#include <memory>

using namespace mw::gdb;

struct exit_stub : break_point
{
    exit_stub() : break_point("_exit")
    {
    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        fr.log() << "***mw-newlib*** Log: Invoking _exit" << std::endl;
        fr.set_exit(std::stoi(fr.arg_list().at(0).value));
    }
};

extern "C" BOOST_SYMBOL_EXPORT std::vector<std::unique_ptr<mw::gdb::break_point>> mw_gdb_setup_bps();

std::vector<std::unique_ptr<mw::gdb::break_point>> mw_gdb_setup_bps()
{
    std::vector<std::unique_ptr<mw::gdb::break_point>> vec;
    vec.push_back(std::make_unique<exit_stub>());
    return vec;
};
