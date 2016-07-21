/**
 * @file   mw/gdb-runner/frame.hpp
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

#ifndef MW_GDB_RUNNER_FRAME_HPP_
#define MW_GDB_RUNNER_FRAME_HPP_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <ostream>
#include <boost/optional.hpp>

namespace mw {
namespace gdb {




struct var
{
    boost::optional<std::uint64_t> ref;
    std::string value;
    std::string policy;
    std::string cstring;
};

struct arg : var
{
    std::string id;

    arg() = default;
    arg(const arg &) = default;
    arg(arg &&) = default;
    arg& operator=(const arg &) = default;
    arg& operator=(arg &&) = default;
    arg(const std::string& id, const std::string& value, const std::string& policy, const std::string& cstring)
        : var{{}, value, policy, cstring}, id(id) {}
};

struct frame
{
    const std::vector<arg> &arg_list() const {return _arg_list;}
    virtual std::unordered_map<std::string, std::uint64_t> regs() = 0;
    virtual void set(const std::string &var, const std::string & val)   = 0;
    virtual void set(const std::string &var, std::size_t idx, const std::string & val) = 0;
    virtual boost::optional<var> call(const std::string & cl)        = 0;
    virtual var print(const std::string & pt)       = 0;
    virtual void return_(const std::string & value) = 0;
    virtual void set_exit(int) = 0;
    virtual std::ostream & log() = 0;
    frame(std::vector<arg> && args)
            : _arg_list(std::move(args))
    {

    }
protected:
    std::vector<arg> _arg_list;

};



} /* namespace gdb_runner */
} /* namespace mw */

#endif /* MW_GDB_RUNNER_FRAME_HPP_ */
