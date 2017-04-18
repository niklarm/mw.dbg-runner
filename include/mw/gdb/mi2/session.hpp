/**
 * @file   /gdb-runner/include/mw/gdb/mi2/session.hpp/session.hpp
 * @date   09.12.2016
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
#ifndef MW_GDB_MI2_SESSION_HPP_
#define MW_GDB_MI2_SESSION_HPP_

#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process/async_pipe.hpp>

#include <string>
#include <ostream>

namespace mw
{
namespace gdb
{
namespace mi2
{

class session
{
    boost::process::async_pipe & _out;
    boost::process::async_pipe & _in;

    boost::asio::streambuf _out_buf;
    std::string _in_buf;
public:
    session(boost::process::async_pipe & out, boost::process::async_pipe & in) : _out(out), _in(in) {}

    void communicate(boost::asio::yield_context & yield_);
    void communicate(const std::string & in, boost::asio::yield_context & yield_);
    void handle();
    void operator()(boost::asio::yield_context & yield_);

    //misc
    void exit();

};



}
}
}



#endif /* MW_GDB_MI2_SESSION_HPP_ */
