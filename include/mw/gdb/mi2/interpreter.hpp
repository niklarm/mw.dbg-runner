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
#include <mw/gdb/mi2/async_record_handler_t.hpp>
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

struct unexpected_result_class : interpreter_error
{
    unexpected_result_class(result_class ex, result_class got) :
        interpreter_error("unexpected result-class [" + to_string(ex) + " != " + to_string(got) + "]")
    {

    }
};

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

    async_record_handler_t async_record_handler{_async_sink};

    void break_after(int number, int count);
    void break_commands(int number, const std::vector<std::string> & commands);
    void break_condition(int number, const std::string & condition);
    void break_delete(int number);
    void break_delete(const std::vector<int> &numbers);

    void break_disable(int number);
    void break_disable(const std::vector<int> &numbers);

    void break_enable(int number);
    void break_enable(const std::vector<int> &numbers);

    breakpoint break_info(int number);

    breakpoint break_insert(const linespec_location & exp,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    breakpoint break_insert(const explicit_location & exp,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    breakpoint break_insert(const address_location & exp,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    breakpoint break_insert(const std::string & location,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const linespec_location & location,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const explicit_location& location,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const address_location & location,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const boost::optional<std::string> & location = boost::none,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    std::vector<breakpoint> break_list();
    void break_passcount(std::size_t tracepoint_number, std::size_t passcount);
    watchpoint break_watch(const std::string & expr, bool access = false, bool read = false);

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
