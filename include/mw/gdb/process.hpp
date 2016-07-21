/**
 * @file   mw/gdb-runner/process.hpp
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

#ifndef MW_GDB_RUNNER_GDB_PROCESS_H_
#define MW_GDB_RUNNER_GDB_PROCESS_H_


#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <mw/gdb/break_point.hpp>
#include <boost/process/child.hpp>
#include <boost/process/async_pipe.hpp>

#include <boost/spirit/home/support/multi_pass.hpp>
#include <iterator>

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <regex>
#include <map>


namespace mw {
namespace gdb {

class process
{
    int _time_out = 10;
    int _exit_code = -1;
    std::ofstream  _log;

    boost::asio::io_service _io_service;
    boost::asio::deadline_timer _timer{_io_service , boost::posix_time::seconds(_time_out)};
    boost::process::async_pipe _out{_io_service};
    boost::process::async_pipe _in {_io_service};
    boost::asio::streambuf _err;
    boost::asio::streambuf _out_buf;

    boost::process::child _child;


    std::string _remote;
    std::string _program;
    int _pid = -1;
    std::vector<std::uint64_t> _thread_id;
    std::vector<std::string> _args;
    std::vector<std::unique_ptr<break_point>> _break_points;
    std::map<int, break_point*>               _break_point_map;
    void _run_impl(boost::asio::yield_context &yield);
    std::vector<std::string> _get_err_data();
    std::vector<std::string> _read_chunk (boost::asio::yield_context & yield);
    void _read_header(boost::asio::yield_context & yield);
    void _set_timer();
    void _set_breakpoint(break_point & p, boost::asio::yield_context &yield, const std::regex & rx);
    void _handle_breakpoint(boost::asio::yield_context &yield, const std::vector<std::string> & buf, std::smatch & sm);


    void _set_info(const std::string & version, const std::string & toolset, const std::string & config)
    {
        _log << "GDB Version \"" << version << '"' << std::endl;
        _log << "GDB Toolset \"" << toolset << '"' << std::endl;
        _log << "Config      \"" << config  << '"' << std::endl;
    }
    void _terminate()
    {
        _child.terminate();
        std::terminate();
    }
    void _read_info();
    template<typename Yield> void _init_bps(Yield & yield_);
    template<typename Yield> void _start(Yield & yield_);
    template<typename Yield> void _start_remote(Yield & yield_);
    template<typename Yield> void _start_local (Yield & yield_);
    template<typename Yield> void _handle_bps(Yield & yield_);
    bool _exited = false;
public:
    void set_exit(int code)
    {
        _exited=true;
        _log << "Exited with " << code << std::endl;
        _exit_code = code;
    }

    void set_args(const std::vector<std::string> & args)
    {
        _args = args;
    }
    void set_remote(const std::string & remote)
    {
        _remote = remote;
    }
    using buf_iterator = std::istreambuf_iterator<char>;
    using iterator =  boost::spirit::multi_pass<buf_iterator>;

    template<typename Yield>
    iterator _read(const std::string & input, Yield & yield_);
    iterator _begin() {return iterator(&_out_buf);}
    iterator _end()   const {return iterator();}

    std::ostream & log() {return _log;}
    process(const std::string & gdb, const std::string & exe, const std::vector<std::string> & args = {});
    ~process() = default;
    int exit_code() {return _exit_code;}
    void set_log(const std::string & name) {_log.open(name); }
    void set_timeout(int value) {_time_out = value;}
    void add_break_point(std::unique_ptr<break_point> && ptr) { _break_points.push_back(std::move(ptr)); }
    void add_break_points(std::vector<std::unique_ptr<break_point>> && ptrs)
    {
     /*   _break_points.insert(_break_points.end(),
                std::make_move_iterator(ptrs.begin()), std::make_move_iterator(ptrs.end()));*/
        for (auto & in : ptrs)
            _break_points.emplace_back(in.release());
    }
    void run();
};

} /* namespace gdb_runner */
} /* namespace mw */

#endif /* MW_GDB_RUNNER_GDB_PROCESS_H_ */
