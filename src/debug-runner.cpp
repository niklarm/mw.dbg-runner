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
  / |/  |   /\    |/ |/
 /  /   |  (  \   /  |
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
#include <boost/process/search_path.hpp>

#include <boost/scope_exit.hpp>

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
    fs::path my_binary;
    fs::path my_path;


    bool help;
    bool debug;
    string dbg;
    string exe;
    string log;
    string log_level;
    vector<string> args;
    vector<string> dbg_args;
    vector<string> other_cmds;
    vector<fs::path> dlls;

    string remote;
    std::vector<boost::dll::shared_library> plugins;

    std::string source_folder;
    po::options_description desc;
    po::variables_map vm;

    int time_out = -1;

    po::positional_options_description pos;


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
        my_binary = argv[0];
        my_path = my_binary.parent_path();

        using namespace boost::program_options;

        desc.add_options()
           ("lib,B",         value<vector<fs::path>>(&dlls)->multitoken(), "break-point libraries")
           ("response-file", value<string>(), "can be specified with '@name', too")
           ("config-file,C", value<string>(), "config file")
            ;

        po::store(po::command_line_parser(argc, argv).
                  options(desc).extra_parser(at_option_parser).allow_unregistered().run(), vm);

        load_cfg(true);

        po::notify(vm);
        for (auto & dll : dlls)
        {
            if (fs::exists(dll))
                plugins.emplace_back(dll);
            else if (dll.parent_path().empty())
            {
                //check for the local version needed
#if defined(BOOST_WINDOWS_API)
                fs::path p = my_path / ("lib" + dll.string() + ".dll");
#else
                fs::path p = my_path / ("lib" + dll.string() + ".so");
#endif
                if (fs::exists(p))
                    plugins.emplace_back(p);
                else
                    continue;
            }
            else
                continue;

            auto & p = plugins.back();
            if (p.has("mw_dbg_setup_options"))
            {
                auto f = boost::dll::import<void(po::options_description &)>(p, "mw_dbg_setup_options");
                po::options_description po;
                f(po);
                desc.add(std::move(po));
            }
            
        }

        //ok, now load the full thingy
        vm.clear();

        desc.add_options()
            ("help,H",      bool_switch(&help),                               "produce help message")
            ("exe,E",       value<string>(&exe),                              "executable to run")
            ("args,A",      value<vector<string>>(&args),                     "Arguments passed to the target")
            ("dbg,G",       value<string>(&dbg)->default_value("gdb"),        "dbg command"  )
            ("dbg-args,U",  value<vector<string>>(&dbg_args)->multitoken(),   "dbg arguments")
            ("source-dir,S", value<string>(&source_folder),                   "directory to look for source source folder")
            ("other,O",     value<vector<string>>(&other_cmds)->multitoken(), "other arguments")
            ("timeout,T",   value<int>(&time_out),                            "time_out")
            ("log,L",       value<string>(&log),                              "log file")
            ("debug,D",     bool_switch(&debug),                              "output the log data to stderr")
            ("remote,R",    value<string>(&remote),                           "Remote settings")
            ;

        pos.add("dbg", 1).add("exe", 1);

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

    fs::path dbg = opt.dbg;
    if ((opt.vm.count("dbg") == 0))
        dbg = bp::search_path("gdb");
    else if (!fs::exists(dbg) && !fs::exists(dbg = bp::search_path(opt.dbg)))
        std::cerr << "Debugger binary " << dbg << " not found" << std::endl;

    if (!opt.source_folder.empty())
        opt.dbg_args.push_back("--directory=" + opt.source_folder);

    mw::gdb::process proc(dbg, opt.exe, opt.dbg_args);

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
        auto f = boost::dll::import<void(std::vector<std::unique_ptr<mw::debug::break_point>>&)>(lib, "mw_dbg_setup_bps");
        std::vector<std::unique_ptr<mw::debug::break_point>> vec;
        f(vec);
        proc.add_break_points(std::move(vec));
    }
    if (!proc.running())
    {
        std::cerr << "Error launching the debugger process" << std::endl;
        std::cerr << "Debugger: " << dbg
                  << "\nExe: " << opt.exe
                  << "\nArs:";
        for (auto & a : opt.dbg_args)
            std::cerr << " " << a ;
        std::cerr << std::endl;

        return 1;
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
        cerr << "Exception thrown '" << e.what() << "'" << endl;
        return 1;
    }
    catch (...)
    {
        cerr << "Unknown error" << endl;
        return 1;
    }
}
