/**
 * @file   gdb-runner.cpp
 * @date   13.06.2016
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

#include <boost/program_options.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>
#include <iostream>

#include <mw/gdb/process.hpp>

namespace po = boost::program_options;
namespace bp = boost::process;
namespace fs = boost::filesystem;

using namespace std;

struct options_t
{
    bool help;
    bool debug;
    string gdb;
    string exe;
    string log;
    vector<string> gdb_args;
    vector<string> other_cmds;
    vector<fs::path> dlls;

    po::options_description desc;
    po::variables_map vm;

    int time_out = -1;

    options_t(int argc, char** argv)
    {
        using namespace boost::program_options;

        desc.add_options()
           ("help,H",      bool_switch(&help),             "produce help message")
           ("exe,E",       value<string>(&exe)->required(), "executable to run")
           ("gdb,G",       value<string>(&gdb)->default_value("gdb"), "gdb command"  )
           ("gdb-args,A",  value<vector<string>>(&gdb_args)->multitoken(), "gdb arguments")
           ("other,O",     value<vector<string>>(&other_cmds)->multitoken(), "other arguments")
           ("timeout,T",   value<int>(&time_out), "time_out")
           ("log,L",       value<string>(&log), "log file")
           ("debug,D",     bool_switch(&debug), "output the log data to stderr")
           ("lib,L",       value<vector<fs::path>>(&dlls)->multitoken(), "break-point libraries")
            ;

        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
};

int main(int argc, char * argv[])
{
    try {
    options_t opt(argc, argv);


    std::vector<bp::child> other;

    for (auto & o : opt.other_cmds)
        other.emplace_back(o, bp::std_in < bp::null, bp::std_out > bp::null, bp::std_err > bp::null);

    mw::gdb::process proc(opt.gdb, opt.exe, opt.gdb_args);

    if (!opt.log.empty())
        proc.set_log(opt.log);

    //just for me:
    if (opt.debug)
        proc.log().rdbuf(cerr.rdbuf());


    if (opt.dlls.empty())
    {
        proc.log() << "No Dll provided, thus no breakpoints will be executed." << endl;
    }
    for (auto & dll : opt.dlls)
    {
        auto f = boost::dll::import<std::vector<std::unique_ptr<mw::gdb::break_point>>()>(dll, "mw_gdb_setup_bps");
    }

    proc.run();

    proc.log() << "Exited with code: " << proc.exit_code() << endl;

    for (auto & o : other)
        o.terminate();
    }
    catch (std::exception & e)
    {
        cout << e.what() << endl;
    }
 	return 1;
}