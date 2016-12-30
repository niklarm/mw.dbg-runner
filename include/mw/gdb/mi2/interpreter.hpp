/**
 * @file   /mw/gdb/mi2/session.hpp
 * @date   09.12.2016
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
#ifndef MW_GDB_MI2_INTERPRETER_HPP_
#define MW_GDB_MI2_INTERPRETER_HPP_

#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/signals2/signal.hpp>

#include <mw/gdb/mi2/types.hpp>
#include <mw/gdb/mi2/output.hpp>
#include <string>
#include <ostream>
#include <functional>
#include <iostream>

namespace mw
{
namespace gdb
{
namespace mi2
{

struct unexpected_record : interpreter_error
{
    unexpected_record(const std::string & line) : interpreter_error("unexpected record " + line)
    {

    }
};

struct unexpected_async_record : unexpected_record
{
    unexpected_async_record(std::uint64_t token, const std::string & line) : unexpected_record("unexpected async record [" + std::to_string(token) + "]" + line)
    {

    }
};

struct mismatched_token : unexpected_record
{
    mismatched_token(std::uint64_t expected, std::uint64_t got)
        : unexpected_record("Token mismatch: [" + std::to_string(expected) + " != " + std::to_string(got) + "]")
    {

    }
};


class interpreter
{
    boost::process::async_pipe & _out;
    boost::process::async_pipe & _in;

    boost::asio::streambuf _out_buf;
    std::string _in_buf;

    boost::asio::yield_context & _yield;

    std::ostream &_fwd;

    boost::signals2::signal<void(const std::string&)> _stream_console;
    boost::signals2::signal<void(const std::string&)> _stream_log;

    boost::signals2::signal<void(const async_output &)> _async_sink;

    std::uint32_t _token_gen = 0;

    void _handle_stream_output(const stream_record & sr);
    void _handle_async_output(const async_output & ao);
    bool _handle_async_output(std::uint64_t token, const async_output & ao);
    template<typename ...Args>
    void _work_impl(Args&&...args);
    void _work();
    void _work(std::uint64_t token, result_class rc);
  //  void _work(const std::function<void(const result_output&)> & func);
    void _work(std::uint64_t, const std::function<void(const result_output&)> & func);

    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr);
    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                        std::uint64_t expected_token, result_class rc);
//    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const stream_record & sr,
//                        const std::function<void(const result_output&)> & func);
    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                        std::uint64_t expected_token, const std::function<void(const result_output&)> & func);

    std::vector<std::pair<std::uint64_t, std::function<bool(const async_output &)>>> _pending_asyncs;
public:
    interpreter(boost::process::async_pipe & out,
                boost::process::async_pipe & in,
                boost::asio::yield_context & yield_,
                std::ostream & fwd = std::cout) : _out(out), _in(in), _yield(yield_), _fwd(fwd) {}

    boost::signals2::signal<void(const std::string&)> & stream_console_sig() {return _stream_console;}
    boost::signals2::signal<void(const std::string&)> & stream_log_sig() {return _stream_log;}


    void communicate(boost::asio::yield_context & yield_);
    void communicate(const std::string & in, boost::asio::yield_context & yield_);
    void handle();
    void operator()(boost::asio::yield_context & yield_);

    void exit();
    void set (const std::string & name, const std::string & value);
    std::string show(const std::string & name);
    void version();
    std::vector<groups> list_thread_groups(bool available = false, int recurse = 0);

    os_info info_os();

    std::string add_inferior();
    void inferior_tty_set(const std::string & str);

    void enable_timings(bool ena = true);
};



}
}
}



#endif /* MW_GDB_MI2_SESSION_HPP_ */
