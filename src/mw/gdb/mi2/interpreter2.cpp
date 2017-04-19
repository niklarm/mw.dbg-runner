/**
 * @file   mw/gdb/mi2/interpreter2.cpp
 * @date   20.04.2017
 * @author Klemens D. Morgenstern
 *
 * Published under [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html)
  <pre>
    /  /|  (  )   |  |  /
   /| / |   \/    | /| /
  / |/  |   /\    |/ |/
 /  /   |  (  \   /  |
               )
 </pre>
 */



#define BOOST_COROUTINE_NO_DEPRECATION_WARNING
#include <mw/gdb/mi2/interpreter.hpp>
#include <mw/gdb/mi2/output.hpp>
#include <mw/gdb/mi2/input.hpp>

#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>


#include <boost/algorithm/string/predicate.hpp>
#include <istream>
#include <iostream>
#include <sstream>

namespace asio = boost::asio;

namespace mw
{
namespace gdb
{
namespace mi2
{

interpreter::interpreter(
            boost::process::async_pipe & out,
            boost::process::async_pipe & in,
            boost::asio::yield_context & yield_,
            std::ostream & fwd) : mw::debug::interpreter_impl(out, in, yield_, fwd) {}

void interpreter::_throw_unexpected_result(result_class rc, const mw::gdb::mi2::result_output & res)
{
    if (res.class_ == result_class::error)
    {
        if (auto cp = find_if(res.results, "msg"))
            BOOST_THROW_EXCEPTION( unexpected_result_class(result_class::done, res.class_, cp->as_string()));
        else
            BOOST_THROW_EXCEPTION( unexpected_result_class(result_class::done, res.class_));
    }
    else
        BOOST_THROW_EXCEPTION( unexpected_result_class(result_class::done, res.class_));
}

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

}}}
