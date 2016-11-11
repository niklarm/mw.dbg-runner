/**
 * @file   mw/gdb/location.hpp
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
#ifndef MW_GDB_LOCATION_HPP_
#define MW_GDB_LOCATION_HPP_

#include <string>

namespace mw
{
namespace gdb
{
///Simple structure representing a location in code
struct location
{
    std::string file; ///<The file the location is in.
    int line;         ///<The line in the file.
};

}
}


#endif /* MW_GDB_LOCATION_HPP_ */
