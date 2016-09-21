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
#include <mw/gdb/detail/frame_impl.hpp>

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


auto process::_read(const std::string & input, boost::asio::yield_context & yield_) -> iterator
{
    this->_set_timer();
    if (_enable_debug)
        cout << input;

    asio::async_write(_in, asio::buffer(input), yield_);
    asio::async_read_until(_out, _out_buf, "(gdb)", yield_);

    if (_enable_debug)
    {
        boost::asio::streambuf::const_buffers_type bufs = _out_buf.data();
        cout << std::string (boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + _out_buf.size());
    }

    return iterator(&_out_buf);
}



process::process(const std::string & gdb, const std::string & exe, const std::vector<std::string> & args)
    : _child(
            boost::filesystem::exists(gdb) ? gdb : bp::search_path(gdb),
            exe, args, _io_service, bp::std_in < _in, bp::std_out > _out, bp::std_err > _err,
            bp::on_exit([this](int, const std::error_code&){_timer.cancel();_out.async_close(); _err.async_close();}))
{
}

vector<string> process::_get_err_data(boost::asio::yield_context & yield_)
{
    size_t sz = 0;

    while (sz != _err_vec.size())
    {
        sz = _err_vec.size();
        _io_service.post(yield_);
    }

    return std::move(_err_vec);
}


void process::_read_info()
{

    if (_enable_debug)
    {
        boost::asio::streambuf::const_buffers_type bufs = _out_buf.data();
        cout << std::string (boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + _out_buf.size());
    }

    namespace p = mw::gdb::parsers;
    info i;
    auto itr = iterator(&_out_buf);
    if(x3::phrase_parse(itr, iterator(), p::info, x3::space, i))
        _set_info(i.version, i.toolset, i.config);
    else
    {
        _log << "Header Parse failed." << endl;
        _terminate();
    }



}

void process::_init_bps(boost::asio::yield_context & yield_)
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
            auto err = this->_get_err_data(yield_);
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

void process::_start(boost::asio::yield_context & yield_)
{
    if (_remote.empty())
        _start_local(yield_);
    else
        _start_remote(yield_);

}

void process::_start_local(boost::asio::yield_context & yield_)
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

void process::_start_remote(boost::asio::yield_context & yield_)
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

void process::_handle_bps(boost::asio::yield_context &yield_)
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
                    detail::frame_impl fr(std::move(b.name), std::move(b.args), *this, itr, yield_, _log);
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
   if (_enable_debug)
       cout << "quit\n\n";

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

void process::_err_read_handler(const boost::system::error_code & ec)
{
    std::istream is(&_err_buf);
    std::string line;

    while (std::getline(is, line))
    {
        if (_enable_debug)
            cerr << line << endl;
        _err_vec.push_back(std::move(line));
    }
    if (!ec)
        boost::asio::async_read_until(_err, _err_buf, '\n',
            [this](const boost::system::error_code& ec,
                    std::size_t bytes_transferred)
                    {
                        _err_read_handler(ec);
                    });
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


    //start
    boost::asio::async_read_until(_err, _err_buf, '\n',
                [this](const boost::system::error_code& ec,
                        std::size_t bytes_transferred)
                        {
                            _err_read_handler(ec);
                        });

    boost::asio::spawn(_io_service, [this](boost::asio::yield_context yield){_run_impl(yield);});
    _io_service.run();

}


} /* namespace gdb_runner */
} /* namespace mw */
