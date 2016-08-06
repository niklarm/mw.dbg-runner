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
#include <mw/gdb/location.hpp>

namespace mw {
namespace gdb {

struct cstring_t
{
    std::string value;
    bool ellipsis;
};


struct var
{
    boost::optional<std::uint64_t> ref;
    std::string value;
    std::string policy;
    cstring_t   cstring;
};

struct arg : var
{
    std::string id;

    arg() = default;
    arg(const arg &) = default;
    arg(arg &&) = default;
    arg& operator=(const arg &) = default;
    arg& operator=(arg &&) = default;
    arg(const std::string& id, const std::string& value, const std::string& policy, const cstring_t& cstring)
        : var{{}, value, policy, cstring}, id(id) {}
};

struct backtrace_elem
{
    int cnt;
    boost::optional<std::uint64_t> call_site;
    std::string func;
    std::string args;
    location loc;
};

struct frame
{
    const std::vector<arg> &arg_list() const {return _arg_list;}
    const arg &arg_list(std::size_t index) const {return _arg_list.at(index);}
    inline std::string get_cstring(std::size_t index);
    virtual std::unordered_map<std::string, std::uint64_t> regs() = 0;
    virtual void set(const std::string &var, const std::string & val)   = 0;
    virtual void set(const std::string &var, std::size_t idx, const std::string & val) = 0;
    virtual boost::optional<var> call(const std::string & cl)        = 0;
    virtual var print(const std::string & pt)       = 0;
    virtual void return_(const std::string & value = "") = 0;
    virtual void set_exit(int) = 0;
    virtual void select(int frame) = 0;

    virtual std::vector<backtrace_elem> backtrace() = 0;

    virtual std::ostream & log() = 0;
    frame(std::vector<arg> && args)
            : _arg_list(std::move(args))
    {

    }
    virtual ~frame() = default;
protected:
    std::vector<arg> _arg_list;

};

std::string frame::get_cstring(std::size_t index)
{
    auto &entry = arg_list(index);

    if (entry.cstring.ellipsis)
        return entry.cstring.value;
    //has ellipsis, so I'll need to get the rest manually
    auto val = entry.cstring.value;
    auto idx = val.size();

    while(true)
    {
        auto p = print(entry.id + '[' + std::to_string(idx++) + ']');
        auto i = std::stoi(p.value);
        auto value  = static_cast<char>(i);
        if (value == '\0')
            break;
        else
            val.push_back(value);
    }
    return val;
}

} /* namespace gdb_runner */
} /* namespace mw */

#endif /* MW_GDB_RUNNER_FRAME_HPP_ */
