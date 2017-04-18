/**
 * @file   test_runner.cpp
 * @date   28.06.2016
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
 */

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/core/lightweight_test.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>

using namespace std;
namespace bp = boost::process;
namespace fs = boost::filesystem;

int main(int argc, char * argv[])
{
    vector<fs::path> vec(argv + 1, argv+argc);

    for (auto & v : vec)
        cout << v << endl;

    auto itr = find_if(vec.begin(), vec.end(),
                       [](fs::path & p)
                       {
                            return p.extension() == ".dll";
                       });

    fs::path dll;

    if (itr != vec.end())
        dll = *itr;
    else
    {
        cout << "No dll found" << endl;
        return 1;
    }

    itr = find_if(vec.begin(), vec.end(),
                       [](fs::path & p)
                       {
                            return p.filename() == "mw-dbg-runner.exe";
                       });
    fs::path exe;

    if (itr != vec.end())
        exe = *itr;
    else
    {
        cout << "No exe found" << endl;
        return 1;
    }

    itr = find_if(vec.begin(), vec.end(),
                       [](fs::path & p)
                       {
                            return p.filename() == "target.exe";
                       });
    fs::path target;

    if (itr != vec.end())
        target = *itr;
    else
    {
        cout << "No target found" << endl;
        return 1;
    }

    cout << "Dll:    " << dll    << endl;
    cout << "Exe:    " << exe    << endl;
    cout << "Target: " << target << endl;

    BOOST_TEST(boost::filesystem::exists(dll));
    BOOST_TEST(boost::filesystem::exists(exe));
    BOOST_TEST(boost::filesystem::exists(target));
    {
        cerr << "\n--------------------------- No-Plugin launch    -----------------------------" << endl;
        auto ret = bp::system(exe,  "--exe=" + target.string(), "--debug", "--timeout=5");
        cerr << "\n-------------------------------------------------------------------------------\n" << endl;

        BOOST_TEST(ret == 0b11111);
        if (ret != 0b11111)
        {
            std::cerr << "Return value Error [" << ret << " != " << 0b11111 << "]" << std::endl;
        }
    }
    {
        cerr << "---------------------------    Plugin launch    -----------------------------" << endl;
        auto ret = bp::system(exe,  "--exe=" + target.string(), "--debug", "--timeout=5", "--lib=" + dll.string());
        cerr << "\n-------------------------------------------------------------------------------\n" << endl;

        BOOST_TEST(ret == 0);

        if (ret != 0)
        {
            std::cerr << "Return value Error [" << ret << " != " << 0 << "]" << std::endl;
        }

    }

    return boost::report_errors();
}
