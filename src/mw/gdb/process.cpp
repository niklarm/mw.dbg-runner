/**
 * @file   mw/gdb-runner/process.cpp
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

#include <mw/gdb/process.hpp>
#include <mw/gdb/parsers.hpp>

#include <boost/variant/get.hpp>
#include <boost/process/io.hpp>
#include <boost/process/async.hpp>
#include <boost/process/search_path.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <iostream>
#include <regex>
#include <tuple>
#include <sstream>
#include <atomic>
#include <algorithm>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


using namespace std;
namespace bp = boost::process;
namespace asio = boost::asio;


namespace x3 = boost::spirit::x3;
namespace mwp = mw::gdb::parsers;


namespace mw {
namespace gdb {


template<typename Yield>
auto process::_read(const std::string & input, Yield & yield_) -> iterator
{
    this->_set_timer();
    asio::async_write(_in, asio::buffer(input), yield_);
    asio::async_read_until(_out, _out_buf, "(gdb)", yield_);
    return iterator(&_out_buf);
}


struct frame_impl : frame
{
    std::unordered_map<std::string, std::uint64_t> regs() override
    {
        std::unordered_map<std::string, std::uint64_t> mp;

        itr = proc._read("info regs\n", yield_);
        vector<reg> v;
        if (!x3::phrase_parse(itr, proc._end(), mwp::regs, x3::space, v))
            throw std::runtime_error("Parser error for registers");

        for (auto & r : v)
            mp[r.id] = r.loc;
        return mp;
    }
    void set(const std::string &var, const std::string & val) override
    {
        itr = proc._read("set variable " + var + " = " + val + "\n", yield_);
        if (!x3::phrase_parse(itr, proc._end(), x3::lit("(gdb)"), x3::space))
            throw std::runtime_error("Parser error for setting variable");
    }
    void set(const std::string &var, std::size_t idx, const std::string & val) override
    {
        itr = proc._read("set variable " + var + "[" + std::to_string(idx) + "] = " + val + "\n", yield_);
        if (!x3::phrase_parse(itr, proc._end(), x3::lit("(gdb)"), x3::space))
            throw std::runtime_error("Parser error for setting variable");
    }
    boost::optional<var> call(const std::string & cl) override
    {
        boost::optional<var> val;
        itr = proc._read("call " + cl + "\n", yield_);
        if (!x3::phrase_parse(itr, proc._end(), -mwp::var >> "(gdb)", x3::space, val))
            throw std::runtime_error("Parser error for call command");

        return val;

    }
    var print(const std::string & pt) override
    {
        var val;
        itr = proc._read("print " + pt + "\n", yield_);
        if (!x3::phrase_parse(itr, proc._end(), mwp::var >> "(gdb)", x3::space, val))
            throw std::runtime_error("Parser error for print command");

        return val;
    }
    void return_(const std::string & value) override
    {
        itr = proc._read("return " + value + "\n", yield_);
        if (!x3::phrase_parse(itr, proc._end(), *(!x3::lit("(gdb)") >> x3::char_) >> "(gdb)", x3::space))
            throw std::runtime_error("Parser error for return command");
    }

    frame_impl(std::vector<arg> && args,
               process & proc,
               process::iterator & itr,
               boost::asio::yield_context & yield_,
               std::ostream & log_)
            : frame(std::move(args)), proc(proc), itr(itr), yield_(yield_), _log(log_)
    {
    }
    void set_exit(int code) override
    {
        proc.set_exit(code);
    }

    std::ostream & log() { return _log; }


    process & proc;
    process::iterator & itr;
    boost::asio::yield_context & yield_;
    std::ostream & _log;
};




process::process(const std::string & gdb, const std::string & exe, const std::vector<std::string> & args)
    : _child(bp::search_path(gdb), exe, args, _io_service, bp::std_in < _in, bp::std_out > _out, bp::std_err > _err,
            bp::on_exit([this](int, const std::error_code&){_timer.cancel();}))
{
}

vector<string> process::_get_err_data()
{
    vector<string> data;
    istream is(&_err);

    string line;
    while (getline(is, line))
        data.push_back(move(line));

    return data;
}


void process::_read_info()
{
    iterator itr(&_out_buf);
    namespace p = mw::gdb::parsers;
    info i;
    if(x3::phrase_parse(itr, _end(), p::info, x3::space, i))
        _set_info(i.version, i.toolset, i.config);
    else
    {
        _log << "Header Parse failed." << endl;
        _terminate();
    }
}

template<typename Yield>
void process::_init_bps(Yield & yield_)
{
    namespace p = mw::gdb::parsers;

    for (auto & bp : _break_points)
    {
        _log << "\nSetting Breakpoint " << bp->identifier() << endl;
        auto itr = _read("break " + bp->identifier() + "\n", yield_);
        mw::gdb::bp_decl val;
        if (x3::phrase_parse(itr, _end(), p::bp_decl >> "(gdb)", x3::space, val))
        {
            _break_point_map[val.cnt] = bp.get();
            if (val.locs.which() == 0)
            {

                auto & loc = boost::get<mw::gdb::location>(val.locs);
                bp->set_at(val.ptr, loc.file, loc.line);
                _log << "Set here: " << loc.file << ":" << loc.line << endl << endl;
            }
            else
            {
                auto & loc = boost::get<mw::gdb::bp_mult_loc>(val.locs);
                bp->set_multiple(val.ptr, loc.name, loc.num);
                _log << "Set multiple breakpoints: " << loc.name << ":" << loc.num << endl << endl;
            }
        }
        else if (x3::phrase_parse(itr, _end(), *(!x3::lit("(gdb)") >> x3::char_) >> "(gdb)", x3::space))
        {
            _log << bp->identifier() << endl;
            auto err = this->_get_err_data();
            for (auto & e : err)
                _log << e << endl;
            _log << endl;
        }
        else
        {
            _log << "Parse error during breakpoint declaration of " << bp->identifier() << endl;
            _terminate();
        }
    }
}

template<typename Yield>
void process::_start(Yield & yield_)
{
    if (_remote.empty())
        _start_local(yield_);
    else
        _start_remote(yield_);

}

template<typename Yield>
void process::_start_local(Yield & yield_)
{
    namespace p = mw::gdb::parsers;


    string cmd = "run";

    for (auto & a : _args)
        cmd += " " + a;

    auto itr = _read(cmd +"\n", yield_);

    std::string prog_name;
    if (x3::phrase_parse(itr, _end(), p::start_prog, x3::space, prog_name))
    {
        _log << " Starting program " << prog_name << endl;
        _program = prog_name;
    }
    else
    {
        _log << "Parser Error while trying to start program" << endl;
        _terminate();
    }

}

template<typename Yield>
void process::_start_remote(Yield & yield_)
{
    namespace p = mw::gdb::parsers;

    //connect
    string cmd = "target remote " + _remote;
    auto itr = _read(cmd +"\n", yield_);

    std::string target;
    if (x3::phrase_parse(itr, _end(),
            "Remote" >> x3::lit("debugging") >> "using"
            >> x3::lexeme[+x3::char_("_:A-Za-z0-9")]
            >> x3::omit[*(!x3::lit("(gdb)") >> x3::char_)] //this may already be the first break-point here
            >> "(gdb)"
            , x3::space, target))
    {
        _log << " Connected to remote target " << target << endl;
        _program = "**remote**";
    }
    else
    {
        _log << "Parser Error while trying to establish remote connection" << endl;
        _terminate();
    }
    //halt the thingy

    itr = _read("monitor reset halt\n", yield_);
    if (x3::phrase_parse(itr, _end(), *(!x3::lit("(gdb)") >> x3::char_) >> "(gdb)", x3::space))
        _log << "Reseted the target" << endl;

    itr = _read("load\n", yield_);
    std::string load_log(itr, _end());

    load_log.resize(load_log.size() - 6);
    _log << "\n" << load_log << "\n" << endl;

    itr = _read("continue\n", yield_);

    if (x3::phrase_parse(itr, _end(), x3::lit("Continuing."), x3::space))
        _log << "Starting" << endl;

    {

        for (auto it2 = itr; x3::phrase_parse(itr, _end(),
                x3::lexeme["Note: " >> +(!(x3::lit('\n') | '\r') >> x3::char_)], x3::space); )
            _log << string(it2, itr) << endl;
    }
}

template<typename Yield>
void process::_handle_bps(Yield &yield_)
{
    namespace p = mw::gdb::parsers;
    auto start_thread_l = [this](auto & ctx)
            {
                mw::gdb::thread_info & ti = x3::_attr(ctx);
                _pid = ti.thr;
                _thread_id.push_back(ti.proc);
                _log << "[New Thread " << _pid << ".0x" << hex << ti.proc << dec <<  "]" << endl;
            };
    auto thread = p::start_thread[start_thread_l] | p::switch_thread | p::exit_thread;

    bp_stop b;

    auto itr = _begin();

    //prefixed thread stuff, parse it beforehand.
    while (x3::phrase_parse(itr, _end(), thread, x3::space));

    //parse the break-point entry
    while (x3::phrase_parse(itr, _end(), p::bp_stop, x3::space, b))
    {
        break_point *bp = _break_point_map.at(b.index);

        vector<mw::gdb::arg> a;


        std::atomic<bool> invoked{false};
        asio::spawn(_io_service,
                [&](boost::asio::yield_context yield_)
                {
                    auto & d = b.loc;
                    std::string file = d.file;
                    int line         = d.line;
                    frame_impl fr(std::move(b.args), *this, itr, yield_, _log);
                    try {
                        bp->invoke(fr, file, line);
                    }
                    catch (std::runtime_error & er)
                    {
                        _log << "Parser Error: '" << er.what() << "'" << endl;
                        _terminate();

                    }
                    invoked.store(true);
                });

        while (!invoked.load())
            _io_service.post(yield_);//now let the thing run.

        if (_exited)
            break;

        itr = _read("continue\n", yield_);

        if (!x3::phrase_parse(itr, _end(), x3::lit("Continuing."), x3::space))
        {
            _log << "Continue parser error";
            _terminate();
        }

        //parse if there's more threading stuff
        while (x3::phrase_parse(itr, _end(), thread, x3::space));

    }
}

void process::_run_impl(boost::asio::yield_context &yield_)
{
    namespace p = mw::gdb::parsers;
    using namespace boost::asio;
    using boost::fusion::at_c;

    asio::async_read_until(_out, _out_buf, "(gdb)", yield_);
    _read_info();

    _init_bps(yield_);
   _start(yield_);


   _handle_bps(yield_);

   auto itr = _begin();

   mw::gdb::exit_proc exit;

   if (!_exited && x3::phrase_parse(itr, _end(), mwp::exit_proc, x3::space, exit))
       set_exit(exit.code);
   _set_timer();
   async_write(_in, buffer("quit\n", 5), yield_);

}

void process::_set_timer()
{
    if (_time_out > 0)
    {
        _timer.expires_from_now(boost::posix_time::seconds(_time_out));
        _timer.async_wait([this](const boost::system::error_code & ec)
                        {
                            if (ec == boost::asio::error::operation_aborted)
                                return;
                            if (_child.running())
                                _child.terminate();
                            _io_service.stop();
                            _log << "...Timeout..." << endl;
                        });
    }
}


void process::run()
{
    _set_timer();
    if (!_child.running())
    {
        _log << "Gdb not running" << endl;
        _terminate();
    }
    _set_timer();
    _log << "Starting run" << endl << endl;

    boost::asio::spawn(_io_service, [this](boost::asio::yield_context yield){_run_impl(yield);});
    _io_service.run();

}


} /* namespace gdb_runner */
} /* namespace mw */
