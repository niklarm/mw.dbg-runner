/**
 * @file   mw/debug/interpreter.cpp
 * @date   01.02.2017
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

#include <mw/debug/interpreter_impl.hpp>

#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>

#include <istream>

namespace asio = boost::asio;

namespace mw
{
namespace debug
{

std::vector<std::string> interpreter_impl::communicate(const std::string & in, char delim)
{
    asio::async_write(_in, asio::buffer(_in_buf), _yield);
    asio::async_read_until(_out, _out_buf, delim, _yield);

    std::vector<std::string> ret;

    std::istream out_str(&_out_buf);
    std::string line;

    while (std::getline(out_str, line))
        ret.push_back(std::move(line));

    return ret;
}

std::vector<std::string> interpreter_impl::communicate(const std::string & in, const std::string & delim)
{
    asio::async_write(_in, asio::buffer(_in_buf), _yield);
    asio::async_read_until(_out, _out_buf, delim, _yield);

    std::vector<std::string> ret;

    std::istream out_str(&_out_buf);
    std::string line;

    while (std::getline(out_str, line))
        ret.push_back(std::move(line));

    return ret;
}

std::vector<std::string> interpreter_impl::communicate(const std::string & in, const boost::regex & expr)
{
    asio::async_write(_in, asio::buffer(_in_buf), _yield);
    asio::async_read_until(_out, _out_buf, expr, _yield);

    std::vector<std::string> ret;

    std::istream out_str(&_out_buf);
    std::string line;

    while (std::getline(out_str, line))
        ret.push_back(std::move(line));

    return ret;
}


}
}
