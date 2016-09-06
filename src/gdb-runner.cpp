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
#include <boost/process/group.hpp>
#include <boost/process/io.hpp>
#include <boost/dll.hpp>
#include <boost/tokenizer.hpp>
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
    string log_level;
    vector<string> args;
    vector<string> gdb_args;
    vector<string> other_cmds;
    vector<fs::path> dlls;

    string remote;

    po::options_description desc;
    po::variables_map vm;

    int time_out = -1;

    po::positional_options_description pos;

    std::vector<boost::dll::shared_library> plugins;

    static pair<string, string> at_option_parser(string const&s)
    {
        if ('@' == s[0])
            return std::make_pair(string("response-file"), s.substr(1));
        else
            return pair<string, string>();
    }

    void load_cfg(bool allow_unregistered = false)
    {
        if (vm.count("response-file"))
        {
             // Load the file and tokenize it
             ifstream ifs(vm["response-file"].as<string>().c_str());
             if (!ifs)
             {
                 cout << "Could not open the response file\n";
                 exit(1);
             }
             // Read the whole file into a string
             stringstream ss;
             ss << ifs.rdbuf();
             // Split the file content
             boost::char_separator<char> sep(" \n\r");
             std::string ResponsefileContents( ss.str() );
             boost::tokenizer<boost::char_separator<char> > tok(ResponsefileContents, sep);
             vector<string> args(tok.begin(), tok.end());
             // Parse the file and store the options
             if (allow_unregistered)
                 po::store(po::command_line_parser(args).options(desc).allow_unregistered().run(), vm);
             else
                 po::store(po::command_line_parser(args).options(desc).run(), vm);
        }
        if (vm.count("config-file"))
        {
            ifstream ifs(vm["config-file"].as<string>());
            po::store(po::parse_config_file(ifs, desc, allow_unregistered), vm);
        }
    }
    void parse(int argc, char** argv)
    {
        using namespace boost::program_options;

        desc.add_options()
           ("lib,B",       value<vector<fs::path>>(&dlls)->multitoken(), "break-point libraries")
           ("response-file", value<string>(), "can be specified with '@name', too")
           ("config-file,C", value<string>(), "config file")
            ;

        load_cfg(true);

        for (auto & dll : dlls)
        {
            plugins.emplace_back(dll);
            auto & p = plugins.back();
            if (p.has("mw_gdb_setup_options"))
            {
                auto f = boost::dll::import<po::options_description()>(p, "mw_gdb_setup_options");
                desc.add(f());
            }
        }

        //ok, now load the full thingy
        vm.clear();

        desc.add_options()
            ("help",        bool_switch(&help), "produce help message")
            ("exe,E",       value<string>(&exe),             "executable to run")
            ("args,A",      value<vector<string>>(&args),   "Arguments passed to the target")
            ("gdb,G",       value<string>(&gdb)->default_value("gdb"), "gdb command"  )
            ("gdb-args,S",  value<vector<string>>(&gdb_args)->multitoken(), "gdb arguments")
            ("other,O",     value<vector<string>>(&other_cmds)->multitoken(), "other arguments")
            ("timeout,T",   value<int>(&time_out),       "time_out")
            ("log,L",       value<string>(&log),         "log file")
            ("debug,D",     bool_switch(&debug),         "output the log data to stderr")
            ("remote,R",    value<string>(&remote), "Remote settings")
            ;

        pos.add("gdb", 1).add("exe", 1);

        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(pos).extra_parser(at_option_parser).run(), vm);

        load_cfg();


        po::notify(vm);
    }
};

int main(int argc, char * argv[])
{
    options_t opt;

    try {
    opt.parse(argc, argv);

    if (opt.help)
    {
        cout << opt.desc << endl;

        return 0;
    }

    if (opt.exe.empty())
    {
        cout << "No executable defined\n" << endl;
        return 1;
    }

    std::vector<bp::child> other;
    bp::group other_group;

    {
        int cnt = 0;
        for (auto & o : opt.other_cmds)
        {
            auto idx = o.find(' ');
            std::string log;
            if (idx == std::string::npos)
                log = o;
            else
                log = o.substr(0, idx);

            boost::algorithm::replace_all(log, "/", "~");
            boost::algorithm::replace_all(log, "\\", "~");
            log += "_" + std::to_string(cnt++);
            other.emplace_back(o, bp::std_in < bp::null, bp::std_out > log, bp::std_err > log, other_group);
        }
    }

    mw::gdb::process proc(opt.gdb, opt.exe, opt.gdb_args);

    if (!opt.log.empty())
        proc.set_log(opt.log);

    if (!opt.args.empty())
        proc.set_args(opt.args);
    //just for me:
    if (opt.debug)
        proc.enable_debug();

    if (opt.dlls.empty())
        proc.log() << "No Dll provided, thus no breakpoints will be used." << endl;

    if (!opt.remote.empty())
        proc.set_remote(opt.remote);

    for (auto & lib : opt.plugins)
    {
        auto f = boost::dll::import<std::vector<std::unique_ptr<mw::gdb::break_point>>()>(lib, "mw_gdb_setup_bps");
        proc.add_break_points(f());
    }

    proc.run();

    proc.log() << "Exited with code: " << proc.exit_code() << endl;

    for (auto & o : other)
        if (o.running())
            o.terminate();

    return proc.exit_code();

    }
    catch (std::exception & e)
    {
        cout << e.what() << endl;
        return -1;
    }
    catch (...)
    {
        cout << "Unknown error" << endl;
        return -1;
    }
}
