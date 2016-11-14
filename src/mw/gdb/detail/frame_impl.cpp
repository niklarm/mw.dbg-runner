/**
 * @file   mw/gdb/detail/frame_impl.cpp
 * @date   01.08.2016
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

#include <mw/gdb/detail/frame_impl.hpp>
#include <mw/gdb/parsers.hpp>

using namespace std;
namespace x3 = boost::spirit::x3;
namespace mwp = mw::gdb::parsers;

namespace mw { namespace gdb { namespace detail {

std::unordered_map<std::string, std::uint64_t> frame_impl::regs()
 {
     std::unordered_map<std::string, std::uint64_t> mp;

     itr = proc._read("info regs\n", yield_);
     vector<reg> v;
     if (!x3::phrase_parse(itr, proc._end(), mwp::regs, x3::space, v))
         throw std::runtime_error("Parser error for registers");

     for (auto & r : v)
         mp[r.id] = r.loc;
     return mp;
 }
 void frame_impl::set(const std::string &var, const std::string & val)
 {
     itr = proc._read("set variable " + var + " = " + val + "\n", yield_);
     if (!x3::phrase_parse(itr, proc._end(), x3::lit("(gdb)"), x3::space))
         throw std::runtime_error("Parser error for setting variable");
 }
 void frame_impl::set(const std::string &var, std::size_t idx, const std::string & val)
 {
     itr = proc._read("set variable " + var + "[" + std::to_string(idx) + "] = " + val + "\n", yield_);
     if (!x3::phrase_parse(itr, proc._end(), x3::lit("(gdb)"), x3::space))
         throw std::runtime_error("Parser error for setting variable");
 }
 boost::optional<var> frame_impl::call(const std::string & cl)
 {
     boost::optional<var> val;
     itr = proc._read("call " + cl + "\n", yield_);
     if (!x3::phrase_parse(itr, proc._end(), mwp::var | "(gdb)", x3::space, val))
         throw std::runtime_error("Parser error for call command");

     return val;

 }
 var frame_impl::print(const std::string & pt, bool bitwise)
 {
     var val;
     if (bitwise)
         itr = proc._read("print /t " + pt + "\n", yield_);
     else
         itr = proc._read("print " + pt + "\n", yield_);
     if (!x3::phrase_parse(itr, proc._end(), mwp::var, x3::space, val))
         throw std::runtime_error("Parser error for print command");
     return val;
 }
 void frame_impl::return_(const std::string & value)
 {
     itr = proc._read("return " + value + "\n", yield_);
     if (!x3::phrase_parse(itr, proc._end(), *(!x3::lit("(gdb)") >> x3::char_) >> "(gdb)", x3::space))
         throw std::runtime_error("Parser error for return command");
 }

 void frame_impl::set_exit(int code)
 {
     proc.set_exit(code);
 }

 void frame_impl::select(int frame)
 {
     itr = proc._read("select-frame " + std::to_string(frame) + "\n", yield_);
     if (!x3::phrase_parse(itr, proc._end(), "(gdb)", x3::space))
         throw std::runtime_error("Parser error for return command");
 }

 std::vector<backtrace_elem> frame_impl::backtrace()
 {
     std::vector<backtrace_elem> ret;
     itr = proc._read("backtrace\n", yield_);
     if (!x3::phrase_parse(itr, proc._end(),  mw::gdb::parsers::backtrace >> "(gdb)", x3::space, ret))
                 throw std::runtime_error("Parser error for backtrace command");

     return ret;
 }


}}}


