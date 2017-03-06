/**
 * @file  mw/gdb/detail/frame_impl.hpp
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
#ifndef MW_GDB_DETAIL_FRAME_IMPL_HPP_
#define MW_GDB_DETAIL_FRAME_IMPL_HPP_

#include <mw/gdb/process.hpp>

namespace mw { namespace gdb { namespace mi2 {


mw::debug::var parse_var(interpreter &interpreter_, const std::string & id, std::string value);

struct frame_impl : mw::debug::frame
{
    std::unordered_map<std::string, std::uint64_t> regs() override;
    void set(const std::string &var, const std::string & val) override;
    void set(const std::string &var, std::size_t idx, const std::string & val) override;
    boost::optional<mw::debug::var> call(const std::string & cl) override;
    mw::debug::var print(const std::string & pt, bool bitwise) override;
    void return_(const std::string & value) override;
    frame_impl(std::string &&id,
               std::vector<mw::debug::arg> && args,
               process & proc,
               mi2::interpreter & interpreter,
               std::ostream & log_)
            : mw::debug::frame(std::move(id), std::move(args)), proc(proc), _interpreter(interpreter), _log(log_)
    {
    }
    void set_exit(int code) override;
    void select(int frame) override;
    virtual std::vector<mw::debug::backtrace_elem> backtrace() override;

    std::ostream & log() override { return _log; }

    mw::debug::interpreter & interpreter() override {return _interpreter; }

    process & proc;
    mw::gdb::mi2::interpreter & _interpreter;
    std::ostream & _log;
};



}}}



#endif /* MW_GDB_DETAIL_FRAME_IMPL_HPP_ */
