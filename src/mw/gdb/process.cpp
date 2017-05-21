/**
 * @file   mw/gdb-runner/process.cpp
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

#define BOOST_COROUTINE_NO_DEPRECATION_WARNING
#include <mw/gdb/process.hpp>
#include <mw/gdb/mi2/frame_impl.hpp>

#include <boost/variant/get.hpp>
#include <iostream>
#include <regex>
#include <tuple>
#include <sstream>
#include <atomic>
#include <algorithm>


using namespace std;
namespace bp = boost::process;
namespace asio = boost::asio;


namespace mw {
namespace gdb {

inline std::vector<std::string> set_interpreter_args(const std::vector<std::string> & args)
{
    std::vector<std::string> args_ = args;
    args_.push_back("--interpreter");
    args_.push_back("mi2");
    return args_;
}

process::process(const boost::filesystem::path & gdb, const std::string & exe, const std::vector<std::string> & args)
    : mw::debug::process(gdb, exe, set_interpreter_args(args))
{
}

void process::_run_impl(boost::asio::yield_context &yield_)
{
    mi2::interpreter interpreter{_out, _in, yield_, _log};

    using namespace boost::asio;
    _read_info(interpreter);

    if (!_program.empty()) //empty means it was not changed since starting
        interpreter.file_exec_and_symbols(_program);

    if (!_args.empty())
        interpreter.exec_arguments(_args);

    if (!_remote.empty())
        interpreter.target_select_remote(_remote);

    _init_bps(interpreter);
    _start(interpreter);


    _handle_bps(interpreter);

    _set_timer();

    interpreter.gdb_exit();
    if (_enable_debug)
        cout << "quit\n\n";

}

void process::_read_info(mi2::interpreter & interpreter)
{
    auto val = interpreter.read_header();

    std::regex reg_version{R"_(GNU gdb (\([^)]+\))? ((?:\d+.)+\d+))_"};
    std::regex reg_config {R"_(This GDB was configured as "([\w"\-\= ]+)")_"};

    std::string version, toolset, config;

    std::smatch sm;
    {
        std::sregex_token_iterator itr{val.begin(), val.end(), reg_version};
        std::sregex_token_iterator end{};

        if (itr != end)
            version = *itr;
        if (++itr != end)
            toolset = *itr;
    }
    if (std::regex_search(val, sm, reg_config))
        config = sm.str();

    _set_info(version, toolset, config);
}

void process::_init_bps(mi2::interpreter & interpreter)
{
    for (auto & bp : _break_points)
    {
        _log << "\nSetting Breakpoint " << bp->identifier() << endl;

        try
        {
            auto bpv = interpreter.break_insert(bp->identifier(),
                    false, false, false, false, false,
                    bp->condition());

            auto & b = bpv[0];
            _break_point_map[b.number] = bp.get();

            BOOST_ASSERT(bpv.size() > 0);
            if (bpv.size() == 1)
            {
                std::string file = b.filename ? *b.filename : std::string();
                auto line = b.line ? *b.line : -1;
                bp->set_at(b.addr, file, line);
                _log << "Set here: " << file << ":" << line << endl << endl;

            }
            else
            {
                std::string func = b.original_location ? *b.original_location : std::string();
                bp->set_multiple(b.addr, func, bpv.size() -1);
                _log << "Set multiple breakpoints: " << func << ":" << (bpv.size() -1) << endl << endl;

            }
        }
        catch (mi2::unexpected_result_class & ie) //just ignore it on error
        {
            if (ie.got != mi2::result_class::error)
            {
                _log << "Parse error during breakpoint declaration of " << bp->identifier() << endl;
                throw;
            }
        }
    }
}

void process::_start(mi2::interpreter & interpreter)
{
    if (_init_scripts.empty() && _remote.empty())
        interpreter.exec_run();
    else if (_init_scripts.empty())
        interpreter.exec_continue();

    for (auto & init : _init_scripts)
        interpreter.interpreter_exec("console", init);
}

void process::_handle_bps  (mi2::interpreter & interpreter)
{
    auto val = interpreter.wait_for_stop();
    while(val.reason != "exited")
    {
        if (val.reason != "breakpoint-hit") //temporary
        {
            _log << "unknown stop reason" << std::endl;
            break;
        }

        int num = std::stoi(mi2::find(val.content, "bkptno").as_string());
       // int thread_id = std::stoi(mi2::find(val.second, "thread-id").as_string());
        auto frame = mi2::parse_result<mi2::frame>(mi2::find(val.content, "frame").as_tuple());

        std::string id;
        if (frame.func)
            id = *frame.func;

        std::vector<mw::debug::arg> args;

        if (frame.args)
        {
            auto & a = *frame.args;
            args.reserve(a.size());
            for (auto & aa : a)
            {
                auto arg = mi2::parse_var(interpreter, aa.name, aa.value);

                mw::debug::arg as;
                as.ref     = arg.ref;
                as.value   = arg.value;
                as.cstring = arg.cstring;
                as.id      = aa.name;

                args.push_back(as);
            }

        }
        mi2::frame_impl fi{std::move(id), std::move(args), *this, interpreter, _log};

        std::string file;
        int line = -1;

        if (frame.file)
            file = *frame.file;
        if (frame.line)
            line = *frame.line;

        _break_point_map[num]->invoke(fi, file, line);

        interpreter.exec_continue();

        val = interpreter.wait_for_stop();
    }

    if (val.reason == "exited-normally")
        this->set_exit(0);

    if (val.reason == "exited")
    {
        int exit_code = std::stoi(find(val.content, "exit-code").as_string(), nullptr, 8);
        this->set_exit(exit_code);
    }

}


void process::_set_timer()
{
    if (_time_out > 0)
    {
        _timer.expires_from_now(boost::posix_time::seconds(_time_out));
        _timer.async_wait(
                [this](const boost::system::error_code & ec)
                {
                    if (ec == boost::asio::error::operation_aborted)
                        return;
                    if (_child.running())
                        _child.terminate();
                    _io_service.stop();
                    _log << "...Timeout..." << endl;
                });
    }
}

void process::run()
{
    _set_timer();
    if (!_child.running())
    {
        _log << "Gdb not running" << endl;
        _terminate();
    }
    _set_timer();
    _log << "Starting run" << endl << endl;

    boost::asio::spawn(_io_service, [this](boost::asio::yield_context yield){_run_impl(yield);});
    _io_service.run();

}

} /* namespace gdb_runner */
} /* namespace mw */
