/**
 * @file   mw/debug/plugin.hpp
 * @date   10.11.2016
 * @author Klemens D. Morgenstern
 *
 * Published under [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html)
  <pre>
    /  /|  (  )   |  |  /
   /| / |   \/    | /| /
  / |/  |   /\    |/ |/
 /  /   |  (  \   /  |
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
#include <mw/debug/break_point.hpp>

///This function is the central function needed to provide a break-point plugin.
BOOST_SYMBOL_EXPORT std::vector<std::unique_ptr<mw::debug::break_point>> mw_dbg_setup_bps();
///This function can be used to add program options for the plugin.
BOOST_SYMBOL_EXPORT boost::program_options::options_description mw_dbg_setup_options();


#endif /* MW_GDB_PLUGIN_HPP_ */
