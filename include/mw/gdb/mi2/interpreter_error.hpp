/**
 * @file   /mw/gdb/mi2/interpreter_error.hpp
 * @date   21.12.2016
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
#ifndef MW_GDB_MI2_INTERPRETER_ERROR_HPP_
#define MW_GDB_MI2_INTERPRETER_ERROR_HPP_

#include <exception>
#include <mw/debug/interpreter.hpp>

namespace mw
{
namespace gdb
{
namespace mi2
{

struct interpreter_error : mw::debug::interpreter_error
{
    using mw::debug::interpreter_error::interpreter_error;
    using mw::debug::interpreter_error::operator=;
};

}
}
}



#endif /* MW_GDB_MI2_INTERPRETER_ERROR_HPP_ */
