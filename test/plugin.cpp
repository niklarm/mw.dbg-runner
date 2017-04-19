#include <boost/dll/alias.hpp>
#include <mw/debug/break_point.hpp>
#include <mw/debug/frame.hpp>
#include <mw/debug/plugin.hpp>
#include <vector>
#include <memory>

using namespace mw::debug;


struct f_ptr : break_point
{
    f_ptr() : break_point("f(int*)")
    {

    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        fr.set("p", 0, "1");
        fr.set("p", 1, "2");
        fr.set("p", 2, "3");
    }
};

struct f_ref : break_point
{
    f_ref() : break_point("f(int&)")
    {
    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        fr.set("ref", "42");
    }
};



struct f_ret : break_point
{
    f_ret() : break_point("f()")
    {

    }
    void invoke(frame & fr, const std::string & file, int line) override
    {
        fr.return_("42");
    }
};

void mw_dbg_setup_bps(std::vector<std::unique_ptr<mw::debug::break_point>> & bps)
{
    bps.push_back(std::make_unique<f_ptr>());
    bps.push_back(std::make_unique<f_ref>());
    bps.push_back(std::make_unique<f_ret>());
};


