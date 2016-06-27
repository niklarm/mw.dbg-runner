/**
 * @file   mw/gdb-runner/break_point.hpp
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

#ifndef MW_GDB_RUNNER_BREAK_POINT_HPP_
#define MW_GDB_RUNNER_BREAK_POINT_HPP_

#include <mw/gdb/frame.hpp>
#include <string>
#include <unordered_map>

namespace mw {
namespace gdb {




class break_point
{
    std::string _identifier;
public:
    const std::string& identifier() const {return _identifier;}
    break_point(const std::string & func_name) : _identifier(func_name) {}
    break_point(const std::string & file_name, std::size_t size)
            : _identifier(file_name + ":" + std::to_string(size)) {}
    virtual ~break_point() = default;
    virtual void invoke(frame & fr, const std::string & file, int line) = 0;
    virtual void set_at(std::uint64_t addr, const std::string & file, int line) {};
    virtual void set_multiple(std::uint64_t addr, std::string & name, int line) {};
    virtual void set_not_found() {};
    virtual void stopped(const std::vector<arg> & vec, const std::string & file, int line) {};
    virtual void set_regs(const std::unordered_map<std::string, std::uint64_t>&) {}
    virtual bool registers() const {return false;}
};




} /* namespace gdb_runner */
} /* namespace mw */

#endif /* MW_GDB_RUNNER_BREAK_POINT_HPP_ */
