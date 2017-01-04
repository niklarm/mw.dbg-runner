/**
 * @file   mw/gdb/mi2/interpreter.cpp
 * @date   20.12.2016
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

#include <mw/gdb/mi2/interpreter.hpp>
#include <mw/gdb/mi2/output.hpp>
#include <mw/gdb/mi2/input.hpp>

#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <istream>
#include <sstream>

namespace asio = boost::asio;

namespace mw
{
namespace gdb
{
namespace mi2
{

void interpreter::_handle_stream_output(const stream_record & sr)
{
    switch (sr.type)
    {
    case stream_record::console:
        _stream_console(sr.content);
        break;
    case stream_record::log:
        _stream_log(sr.content);
        break;
    case stream_record::target:
        _fwd << sr.content;
        break;
    }
}

template<typename ...Args>
void interpreter::_work_impl(Args&&...args)
{
    asio::async_write(_in, asio::buffer(_in_buf), _yield);
    asio::async_read_until(_out, _out_buf, "(gdb)", _yield);

    std::istream out_str(&_out_buf);
    std::string line;
    while (std::getline(out_str, line) && !boost::starts_with(line, "(gdb)"))
    {
        //check streamoutput
        if (auto data = parse_stream_output(line))
        {
            _handle_stream_output(*data);
            continue;
        }

        if (auto data = parse_async_output(line))
        {
            if (data->first)
            {
                if (!_handle_async_output(*data->first, data->second))
                    throw unexpected_async_record(*data->first, line);
            }

            else
                _handle_async_output(data->second);
            continue;
        }

        if (auto data = parse_record(line))
        {
            _handle_record(line, data->first, data->second, std::forward<Args>(args)...);
            continue;
        }
        _fwd << line << '\n';
    }
    _fwd << "(gdb)" << std::endl;
}


void interpreter::_work() {_work_impl();}
void interpreter::_work(std::uint64_t token, result_class rc) { _work_impl(token, rc); }
//void interpreter::_work(const std::function<void(const result_output&)> & func) {_work_impl(func); }
void interpreter::_work(std::uint64_t token, const std::function<void(const result_output&)> & func) {_work_impl(token, func);}

void interpreter::_handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr)
{
    throw unexpected_record(line);
}
void interpreter::_handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                    std::uint64_t expected_token, result_class rc)
{
    if (!token)
        throw mismatched_token(expected_token, 0);

    if (*token != expected_token)
        throw mismatched_token(expected_token, *token);

    if (sr.class_ == result_class::error)
    {
        auto err = parse_result<error_>(sr.results);
        throw exception(err);
    }
    if ((sr.class_ == rc) && sr.results.empty())
        return;
    else
        throw unexpected_record(line);
}

void interpreter::_handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                    std::uint64_t expected_token, const std::function<void(const result_output&)> & func)
{
    if (!token)
        throw mismatched_token(expected_token, 0);

    if (*token != expected_token)
        throw mismatched_token(expected_token, *token);

    func(sr);
}


void interpreter::_handle_async_output(const async_output & ao)
{
    _async_sink(ao);
}
bool interpreter::_handle_async_output(std::uint64_t token, const async_output & ao)
{
    int cnt = 0;
    for (auto itr = _pending_asyncs.begin(); itr != _pending_asyncs.end(); itr++)
        if ((itr->first == token))
        {
            cnt++;
            if (itr->second(ao))
                itr = _pending_asyncs.erase(itr) - 1;
        }

    return cnt != 0;
}

/**
 * The breakpoint number number is not in effect until it has been hit count times.
 * To see how this is reflected in the output of the ‘-break-list’ command, see the
 * description of the ‘-break-list’ command below.
 */

void interpreter::break_after(int number, int count)
{
    _in_buf = std::to_string(_token_gen) + "-break-after " + std::to_string(number) + " " + std::to_string(count) + "\n";
    _work(_token_gen++, result_class::done);
}

/**
 * Specifies the CLI commands that should be executed when breakpoint number is hit.
 * The parameters command1 to commandN are the commands. If no command is specified,
 * any previously-set commands are cleared. See Break Commands. Typical use of this
 * functionality is tracing a program, that is, printing of values of some variables
 * whenever breakpoint is hit and then continuing.
 */

void interpreter::break_commands(int number, const std::vector<std::string> & commands)
{
    _in_buf = std::to_string(_token_gen) + "-break-command " + std::to_string(number);

    for (const auto & cmd : commands)
    {
        _in_buf += " \"";
        _in_buf += cmd;
        _in_buf += '"';
    }

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}

/**
 * Breakpoint number will stop the program only if the condition in expr is true.
 * The condition becomes part of the ‘-break-list’ output (see the description of the
 * ‘-break-list’ command below).
 */

void interpreter::break_condition(int number, const std::string & condition)
{
    _in_buf = std::to_string(_token_gen) + "-break-condition " + std::to_string(number) + ' ' + condition + '\n';
    _work(_token_gen++, result_class::done);
}

/**
 * Delete the breakpoint(s) whose number(s) are specified in the argument list.
 * This is obviously reflected in the breakpoint list.
 */
void interpreter::break_delete(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-delete " + std::to_string(number) + '\n';
    _work(_token_gen++, result_class::done);
}
///@overload void interpreter::break_delete(int number)
void interpreter::break_delete(const std::vector<int> &numbers)
{
    _in_buf = std::to_string(_token_gen) + "-break-delete ";

    for (auto & x : numbers)
        _in_buf += " " + std::to_string(x);

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}


/**
 * Disable the named breakpoint(s). The field ‘enabled’ in the break list is now
 * set to ‘n’ for the named breakpoint(s).
 */
void interpreter::break_disable(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-disable " + std::to_string(number) + '\n';
    _work(_token_gen++, result_class::done);
}
///@overload void interpreter::break_delete(int number)
void interpreter::break_disable(const std::vector<int> &numbers)
{
    _in_buf = std::to_string(_token_gen) + "-break-disable ";

    for (auto & x : numbers)
        _in_buf += " " + std::to_string(x);

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}

/**
 * Enable (previously disabled) breakpoint(s).
 */
void interpreter::break_enable(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-enable " + std::to_string(number) + '\n';
    _work(_token_gen++, result_class::done);
}
///@overload void interpreter::break_delete(int number)
void interpreter::break_enable(const std::vector<int> &numbers)
{
    _in_buf = std::to_string(_token_gen) + "-break-enable ";

    for (auto & x : numbers)
        _in_buf += " " + std::to_string(x);

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}


breakpoint interpreter::break_info(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-info " + std::to_string(number) + '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<breakpoint>(rc.results);
}

static std::string loc_for_break(const linespec_location & exp)
{
    std::string location;

    if (exp.linenum && !exp.filename)
        location = std::to_string(*exp.linenum);
    else if (exp.offset)
        location = std::to_string(*exp.offset);
    else if (exp.filename && exp.linenum)
        location = *exp.filename + ":" + std::to_string(*exp.linenum);
    else if (exp.function && !exp.label && !exp.filename)
        location = *exp.function;
    else if (exp.function && exp.label)
        location = *exp.function + ":" + *exp.label;
    else if (exp.filename && exp.function)
        location = *exp.filename + ":" + *exp.function;
    else if (exp.label)
        location = *exp.label;

    return location;
}

static std::string loc_for_break(const explicit_location & exp)
{
    std::string location;
    if (exp.source)
        location += "--source "   + *exp.source   + " ";
    if (exp.function)
        location += "--function " + *exp.function + " ";
    if (exp.label)
        location += "--label " + *exp.label + " ";

    if (exp.line)
        location += "--line " + std::to_string(*exp.line);
    else if (exp.line_offset)
    {
        if (*exp.line_offset > 0)
            location += "--line +" + std::to_string(*exp.line_offset);
        else
            location += "--line " + std::to_string(*exp.line_offset);
    }
    return location;
}

static std::string loc_for_break(const address_location & exp)
{
    if (exp.expression)
        return "*" + *exp.expression;

    std::stringstream location;
    location << "*";

    if (!exp.filename && exp.funcaddr)
        location << "0x" << std::hex << *exp.funcaddr;
    else if (exp.filename && exp.funcaddr)
        location << "'" << *exp.filename << "'" << "0x" << std::hex << *exp.funcaddr;
    return location.str();
}

inline const value& find(const std::vector<result> & input, const char * id)
{
    auto itr = std::find_if(input.begin(), input.end(),
                            [&](const result &r){return r.variable == id;});

    if (itr == input.end())
        throw missing_value(id);

    return itr->value_;
}

breakpoint interpreter::break_insert(const linespec_location & exp,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return break_insert(loc_for_break(exp), temporary, hardware, pending, disabled, tracepoint, condition, ignore_count, thread_id);
}

breakpoint interpreter::break_insert(const explicit_location & exp,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{


    return break_insert(loc_for_break(exp), temporary, hardware, pending, disabled, tracepoint, condition, ignore_count, thread_id);
}

breakpoint interpreter::break_insert(const address_location & exp,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return break_insert(loc_for_break(exp), temporary, hardware, pending, disabled, tracepoint, condition, ignore_count, thread_id);
}

breakpoint interpreter::break_insert(const std::string & location,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    _in_buf = std::to_string(_token_gen) + "-break-insert ";
    if (temporary)
        _in_buf += "-t ";
    if (hardware)
        _in_buf += "-h ";
    if (pending)
        _in_buf += "-f ";
    if (disabled)
        _in_buf += "-d ";
    if (tracepoint)
        _in_buf += "-a ";
    if (condition)
        _in_buf += "-c " + *condition + " ";
    if (ignore_count)
        _in_buf += "-i " + std::to_string(*ignore_count) + " ";
    if (thread_id)
        _in_buf += "-p " + std::to_string(*thread_id) + " ";

    _in_buf += (location + '\n');

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}


breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument,
        const linespec_location & location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return  dprintf_insert(format, argument, boost::make_optional(loc_for_break(location)),
            temporary, pending, disabled, condition, ignore_count, thread_id);
}

breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument,
        const explicit_location& location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return  dprintf_insert(format, argument, boost::make_optional(loc_for_break(location)),
            temporary, pending, disabled, condition, ignore_count, thread_id);
}

breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument,
        const address_location& location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return  dprintf_insert(format, argument, boost::make_optional(loc_for_break(location)),
            temporary, pending, disabled, condition, ignore_count, thread_id);
}

breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument = {},
        const boost::optional<std::string> & location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    _in_buf = std::to_string(_token_gen) + "-dprintf-insert ";
    if (temporary)
        _in_buf += "-t ";
    if (pending)
        _in_buf += "-f ";
    if (disabled)
        _in_buf += "-d ";
    if (condition)
        _in_buf += "-c " + *condition + " ";
    if (ignore_count)
        _in_buf += "-i " + std::to_string(*ignore_count) + " ";
    if (thread_id)
        _in_buf += "-p " + std::to_string(*thread_id) + " ";

    if (location)
        _in_buf += *location + " ";

    _in_buf += format;

    for (auto & arg : argument)
        _in_buf +=  " " + arg;

    _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<breakpoint>(find(rc.results, "bpkt").as_tuple());
}




std::vector<breakpoint> interpreter::break_list()
{
    mw::gdb::mi2::result_output rc;
    _in_buf = std::to_string(_token_gen) + "-break-list\n";
    _work(_token_gen++,
            [&](const mw::gdb::mi2::result_output & rc_in)
                        {
                            rc = std::move(rc_in);
                        });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    auto body = find(rc.results, "body").as_list().as_results();

    std::vector<breakpoint> vec;
    vec.resize(body.size());

    std::transform(body.begin(), body.end(), vec.begin(),
                    [&](const result & rc)
                    {
                       return parse_result<breakpoint>(find(rc.value_.as_tuple(), "bkpt").as_list().as_results());
                    });
    return vec;
}

void interpreter::break_passcount(std::size_t tracepoint_number, std::size_t passcount)
{
    _in_buf = std::to_string(_token_gen) + "-break-passcount " + std::to_string(tracepoint_number) + ' ' + std::to_string(passcount) + '\n';
    _work(_token_gen++, result_class::done);
}

watchpoint interpreter::break_watch(const std::string & expr, bool access, bool read)
{
    _in_buf = std::to_string(_token_gen) + "-break-watch ";

    if (access)
        _in_buf += "-a ";
    if (read)
        _in_buf += "-r ";

    _in_buf += expr;
    _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<watchpoint>(find(rc.results, "wpt").as_tuple());
}

breakpoint interpreter::catch_load(const std::string regexp,
                       bool temporary, bool disabled)
{
    _in_buf = std::to_string(_token_gen) + "-catch-load ";
    if (temporary)
        _in_buf += "-t ";
    if (disabled)
        _in_buf += "-d ";

    _in_buf += regexp;
    _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

breakpoint interpreter::catch_unload(const std::string regexp,
                                     bool temporary, bool disabled)

{
    _in_buf = std::to_string(_token_gen) + "-catch-unload ";
    if (temporary)
        _in_buf += "-t ";
    if (disabled)
        _in_buf += "-d ";

    _in_buf += regexp;
    _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

breakpoint interpreter::catch_assert(const boost::optional<std::string> & condition, bool temporary, bool disabled)
{
    _in_buf = std::to_string(_token_gen) + "-catch-assert";

    if (condition)
        _in_buf += " -c " + *condition;
    if (temporary)
        _in_buf += " -t";
    if (disabled)
        _in_buf += " -d";

     _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

breakpoint interpreter::catch_exception(const boost::optional<std::string> & condition, bool temporary, bool disabled)
{
    _in_buf = std::to_string(_token_gen) + "-catch-exception";

    if (condition)
        _in_buf += " -c " + *condition;
    if (temporary)
        _in_buf += " -t";
    if (disabled)
        _in_buf += " -d";

     _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

void interpreter::exec_arguments(const std::vector<std::string> & args)
{
    _in_buf = std::to_string(_token_gen) + "-exec-arguments";

    for (const auto & arg : args)
    {
        _in_buf += ' ';
        _in_buf += arg;
    }

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::environment_cd(const std::string path)
{
    _in_buf = std::to_string(_token_gen) + "-environment-cd " + path + '\n' ;
    _work(_token_gen++, result_class::done);
}

std::string interpreter::environment_directory(const std::vector<std::string> & path, bool reset)
{
    std::string result;

    _in_buf = std::to_string(_token_gen) + "-environment-directory";

    if (reset)
        _in_buf += " -r";


    for (auto & p : path)
        _in_buf += (" " + p);

     _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return find(rc.results, "source-path").as_string();
}

std::string interpreter::environment_path(const std::vector<std::string> & path, bool reset)
{
    std::string result;

    _in_buf = std::to_string(_token_gen) + "-environment-path";

    if (reset)
        _in_buf += " -r";


    for (auto & p : path)
        _in_buf += (" " + p);

     _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return find(rc.results, "path").as_string();
}

std::string interpreter::environment_pwd()
{
    std::string result;

    _in_buf = std::to_string(_token_gen) + "-environment-pwd\n";
    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return find(rc.results, "cwd").as_string();
}


thread_state interpreter::thread_info_(const boost::optional<int> & id)
{
    _in_buf = std::to_string(_token_gen) + "-thread-info";
    if (id)
        _in_buf += (" " + std::to_string(*id) );
    _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       throw unexpected_result_class(result_class::done, rc.class_);

   return parse_result<thread_state>(rc.results);
}

thread_id_list interpreter::thread_list_ids()
{
    _in_buf = std::to_string(_token_gen) + "-thread-list-ids\n";


    mw::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       throw unexpected_result_class(result_class::done, rc.class_);

   return parse_result<thread_id_list>(rc.results);
}

thread_select interpreter::thread_select(int id)
{
    _in_buf = std::to_string(_token_gen) + "-thread-select " + std::to_string(id) + '\n';

    mw::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       throw unexpected_result_class(result_class::done, rc.class_);

   return parse_result<struct thread_select>(rc.results);
}

std::vector<ada_task_info> interpreter::ada_task_info(const boost::optional<int> & task_id)
{
    _in_buf = std::to_string(_token_gen) + "-ada-task-info";
    if (task_id)
        _in_buf += " " +  std::to_string(*task_id);
    _in_buf += '\n';

    mw::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       throw unexpected_result_class(result_class::done, rc.class_);

   std::vector<struct ada_task_info> infos;

   for (const auto & v : find(rc.results, "body").as_list().as_values())
       infos.push_back(parse_result<struct ada_task_info>(v.as_tuple()));


   return infos;
}

void interpreter::exec_continue(bool reverse, bool all)
{
    _in_buf = std::to_string(_token_gen) + "-exec-continue";

    if (reverse)
        _in_buf += " --reverse";
    if (all)
        _in_buf += " --all";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_continue(bool reverse, int thread_group)
{
    _in_buf = std::to_string(_token_gen) + "-exec-continue";

    if (reverse)
        _in_buf += " --reverse";
    if (thread_group)
        _in_buf += " --thread-group " + std::to_string(thread_group);
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_finish(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-finish";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_interrupt(bool all)
{
    _in_buf = std::to_string(_token_gen) + "-exec-continue";

    if (all)
        _in_buf += " --all";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_interrupt(int thread_group)
{
    _in_buf = std::to_string(_token_gen) + "-exec-continue";

    if (thread_group)
        _in_buf += " --thread-group " + std::to_string(thread_group);
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}


static std::string loc_str(const linespec_location & exp)
{
    std::string location;

    if (exp.linenum && !exp.filename)
        location = std::to_string(*exp.linenum);
    else if (exp.offset)
        location = std::to_string(*exp.offset);
    else if (exp.filename && exp.linenum)
        location = *exp.filename + ":" + std::to_string(*exp.linenum);
    else if (exp.function && !exp.label && !exp.filename)
        location = *exp.function;
    else if (exp.function && exp.label)
        location = *exp.function + ":" + *exp.label;
    else if (exp.filename && exp.function)
        location = *exp.filename + ":" + *exp.function;
    else if (exp.label)
        location = *exp.label;

    return location;
}

static std::string loc_str(const explicit_location & exp)
{
    std::string location;
    if (exp.source)
        location += "-source "   + *exp.source   + " ";
    if (exp.function)
        location += "-function " + *exp.function + " ";
    if (exp.label)
        location += "-label " + *exp.label + " ";

    if (exp.line)
        location += "-line " + std::to_string(*exp.line);
    else if (exp.line_offset)
    {
        if (*exp.line_offset > 0)
            location += "-line +" + std::to_string(*exp.line_offset);
        else
            location += "-line " + std::to_string(*exp.line_offset);
    }
    return location;
}

static std::string loc_str(const address_location & exp)
{
    if (exp.expression)
        return "*" + *exp.expression;

    std::stringstream location;
    location << "*";

    if (!exp.filename && exp.funcaddr)
        location << "0x" << std::hex << *exp.funcaddr;
    else if (exp.filename && exp.funcaddr)
        location << "'" << *exp.filename << "'" << "0x" << std::hex << *exp.funcaddr;
    return location.str();
}


void interpreter::exec_jump(const linespec_location & ls) {return exec_jump(loc_str(ls));}
void interpreter::exec_jump(const explicit_location & el) {return exec_jump(loc_str(el));}
void interpreter::exec_jump(const address_location  & al) {return exec_jump(loc_str(al));}

void interpreter::exec_jump(const std::string & location)
{
    _in_buf = std::to_string(_token_gen) + "-exec-jump " + location + '\n';
    _work(_token_gen++, result_class::running);

}

void interpreter::exec_next(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-next";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_next_instruction(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-next-instruction";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

frame interpreter::exec_return()
{
    _in_buf = std::to_string(_token_gen) + "-exec-return\n";

    mw::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const mw::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
        throw unexpected_result_class(result_class::done, rc.class_);

    return parse_result<frame>(find(rc.results, "frame").as_tuple());
}

void interpreter::exec_run(bool start, bool all)
{
    _in_buf = std::to_string(_token_gen) + "-exec-run";
    if (start)
        _in_buf += " --start";
    if (all)
        _in_buf += " --all";

    _in_buf += '\n';
    _work(_token_gen++, result_class::running);
}

void interpreter::exec_run(bool start, int thread_group)
{
    _in_buf = std::to_string(_token_gen) + "-exec-run";
    if (start)
        _in_buf += " --start";

    _in_buf += (" thread-group " + std::to_string(thread_group) + '\n');
    _work(_token_gen++, result_class::running);
}

void interpreter::exec_step(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-step";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}
void interpreter::exec_step_instruction(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-step-instruction";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}


void interpreter::exec_until(const linespec_location & ls) {return exec_until(loc_str(ls));}
void interpreter::exec_until(const explicit_location & el) {return exec_until(loc_str(el));}
void interpreter::exec_until(const address_location  & al) {return exec_until(loc_str(al));}

void interpreter::exec_until(const std::string & location)
{
    _in_buf = std::to_string(_token_gen) + "-exec-until " + location + '\n';
    _work(_token_gen++, result_class::running);

}

}
}
}




