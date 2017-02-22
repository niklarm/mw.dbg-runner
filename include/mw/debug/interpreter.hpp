/**
 * @file   mw/debug/interpreter.hpp
 * @date   01.02.2017
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
#ifndef MW_DEBUG_INTERPRETER_HPP_
#define MW_DEBUG_INTERPRETER_HPP_

#include <stdexcept>

#include <boost/regex.hpp>

namespace mw
{

namespace debug
{

struct interpreter_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
    using std::runtime_error::operator=;
};

class interpreter
{
public:
    interpreter() = default;

    virtual ~interpreter() = default;
};

}

}



#endif /* MW_DEBUG_INTERPRETER_HPP_ */
