/**
 * @file   mw/gdb/plugin.hpp
 * @date   10.11.2016
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

This header provides the function declarations needed to create plugins. It should be included
to asser the correctness of the function signatures and linkage.

 */
#ifndef MW_GDB_PLUGIN_HPP_
#define MW_GDB_PLUGIN_HPP_

#include <vector>
#include <memory>
#include <boost/program_options/options_description.hpp>

///This function is the central function needed to provide a break-point plugin.
extern "C" BOOST_SYMBOL_EXPORT std::vector<std::unique_ptr<mw::gdb::break_point>> mw_gdb_setup_bps();
///This function can be used to add program options for the plugin.
extern "C" BOOST_SYMBOL_EXPORT boost::program_options::options_description mw_gdb_setup_options();

#endif /* MW_GDB_PLUGIN_HPP_ */
