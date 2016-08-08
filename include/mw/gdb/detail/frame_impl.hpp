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

namespace mw { namespace gdb { namespace detail {

struct frame_impl : frame
{
    std::unordered_map<std::string, std::uint64_t> regs() override;
    void set(const std::string &var, const std::string & val) override;
    void set(const std::string &var, std::size_t idx, const std::string & val) override;
    boost::optional<var> call(const std::string & cl) override;
    var print(const std::string & pt) override;
    void return_(const std::string & value) override;
    frame_impl(std::vector<arg> && args,
               process & proc,
               process::iterator & itr,
               boost::asio::yield_context & yield_,
               std::ostream & log_)
            : frame(std::move(args)), proc(proc), itr(itr), yield_(yield_), _log(log_)
    {
    }
    void set_exit(int code) override;
    void select(int frame) override;
    virtual std::vector<backtrace_elem> backtrace() override;

    std::ostream & log() { return _log; }


    process & proc;
    process::iterator & itr;
    boost::asio::yield_context & yield_;
    std::ostream & _log;
};



}}}



#endif /* MW_GDB_DETAIL_FRAME_IMPL_HPP_ */
