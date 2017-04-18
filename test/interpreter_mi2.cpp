/**
 * @file   /gdb-runner/test/interpreter.cpp
 * @date   21.02.2017
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

#include <boost/optional/optional_io.hpp>
#include <mw/gdb/mi2/interpreter.hpp>
#include <boost/variant/get.hpp>
#include <boost/process.hpp>

#define BOOST_TEST_MODULE interpreter_mi2_test

#include <boost/test/unit_test.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <sstream>
#include <typeinfo>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/core/demangle.hpp>

#include <boost/algorithm/string/predicate.hpp>

namespace bp = boost::process;
namespace mi2 = mw::gdb::mi2;

struct MyProcess
{

    char* target()
    {
        using boost::unit_test::framework::master_test_suite;

        BOOST_ASSERT(master_test_suite().argc > 1);
        return master_test_suite().argv[1];
    }

    boost::asio::io_service ios;
    bp::async_pipe out{ios};
    bp::async_pipe in {ios};
    bp::async_pipe err{ios};
    bp::child ch{bp::search_path("gdb"), bp::args={target(), "--interpreter=mi2"}, ios, bp::std_in < in, bp::std_out > out, bp::std_err > err};

    //std::thread thr{[this]{ios.run();}};
    std::atomic<bool> done{false};
    std::stringstream ss_err;

    bool first_run = true;
    static MyProcess * proc;

    MyProcess()
    {
        BOOST_ASSERT(ch.running());
        proc = this;
        std::cout << "starting process\n";
    }
    ~MyProcess()
    {
        std::cout << "terminating process\n";
        if (ch.running())
            ch.terminate();
        ch.wait();
        //thr.join();
    }

    template<typename T>
    void run(T & func)
    {
        boost::asio::spawn(ios,
                [&](boost::asio::yield_context yield_)
                {
                    mw::gdb::mi2::interpreter intp{out, in, yield_, ss_err};
                    if (first_run == true)
                    {
                        intp.read_header();
                        first_run = false;
                    }

                    func(intp);
                    done.store(true);
                    BOOST_TEST_MESSAGE("Internal log of interpreter \n---------------------------------------------------------------------------\n"
                                       + ss_err.str() +
                                       "\n---------------------------------------------------------------------------\n");
                    ss_err.clear();
                });
        while (!done.load())
            ios.poll_one();

        done.store(false);
    }

};

MyProcess * MyProcess::proc = nullptr;

#define MW_TEST_CASE(...) \
void BOOST_PP_CAT(mw_test_,  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__) ) (mw::gdb::mi2::interpreter & mi); \
BOOST_AUTO_TEST_CASE (  __VA_ARGS__) \
{ \
    BOOST_REQUIRE(MyProcess::proc != nullptr); \
    MyProcess::proc->run( BOOST_PP_CAT(mw_test_,  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__) ) ) ; \
} \
void BOOST_PP_CAT(mw_test_,  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__) ) (mw::gdb::mi2::interpreter & mi)

BOOST_GLOBAL_FIXTURE(MyProcess);

MW_TEST_CASE( create_bp )
{
    BOOST_CHECK_THROW(mi.break_after(42, 2), mw::gdb::mi2::exception);
    BOOST_TEST_PASSPOINT();

    try {
        mi2::linespec_location ll;
        ll.linenum  = 34;
        ll.filename = "target.cpp";

        auto bp1 = mi.break_insert(ll);
        BOOST_CHECK(true);
        BOOST_TEST_PASSPOINT();
        BOOST_REQUIRE_EQUAL(bp1.size(), 1u);

        mi2::linespec_location ex;
        ex.function = "f";

        decltype(bp1) bp2;
        BOOST_REQUIRE_NO_THROW(bp2 = mi.break_insert(ex));
        BOOST_TEST_PASSPOINT();

        BOOST_REQUIRE_GE(bp2.size(), 1u);

        auto bp = bp1.front();

        BOOST_CHECK_NO_THROW(mi.break_after(bp.number, 2));
        BOOST_CHECK_NO_THROW(mi.break_disable(bp.number));
        BOOST_CHECK_NO_THROW(BOOST_CHECK_EQUAL(mi.break_info(bp.number).number, bp.number));
        BOOST_CHECK_NO_THROW(mi.break_enable(bp.number));

        BOOST_REQUIRE_GE(bp2.size(), 1u);

        BOOST_CHECK_NO_THROW(mi.break_delete({bp.number, bp2.front().number}));

        BOOST_CHECK_NO_THROW(mi.exec_run());


        mi2::async_result ar;
        BOOST_CHECK_NO_THROW(ar = mi.wait_for_stop());

        BOOST_CHECK_EQUAL(ar.reason, "exited");

        BOOST_CHECK_NO_THROW(mi.gdb_exit());

        BOOST_CHECK_THROW(mi.gdb_exit(), boost::system::system_error);
    }
    catch (std::exception & e)
    {
        std::cerr << "Exception '" << e.what() << std::endl;
        std::cerr << "Ex-Type: '" << boost::core::demangle(typeid(e).name()) << "'" << std::endl;
        BOOST_CHECK(false);
    }
}

