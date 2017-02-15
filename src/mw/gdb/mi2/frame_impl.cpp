/**
 * @file   mw/gdb/mi2/frame_impl.cpp
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

#include <mw/gdb/mi2/frame_impl.hpp>
#include <mw/gdb/mi2/interpreter.hpp>
#include <algorithm>

#include <pegtl.hh>

using namespace std;

namespace mw { namespace gdb { namespace mi2 {

namespace parser
{

template< typename Rule >
struct action
   : pegtl::nothing< Rule > {};


struct ellipsis  : pegtl::string<'.', '.', '.'> {};

template<>
struct action<ellipsis> : pegtl::normal<ellipsis>
{
    template<typename Input>
    static void apply(const Input & input, mw::debug::var & v)
    {
        v.cstring.ellipsis = true;
    }
};


struct cstring_body : pegtl::star<
                pegtl::sor<
                    pegtl::string<'\\', '"'>,
                    pegtl::minus<pegtl::any, pegtl::one<'"'>>
                    >>
{

};

template<>
struct action<cstring_body> : pegtl::normal<cstring_body>
{
    template<typename Input>
    static void apply(const Input & input, mw::debug::var & v)
    {
        v.cstring.value = input.string();
    }
};

struct cstring_t :
        pegtl::seq<
            pegtl::one<'"'>,
            cstring_body,
            pegtl::one<'"'>,
            pegtl::opt<ellipsis>
            >
{};



struct global_offset :
        pegtl::seq<
            pegtl::one<'<'>,
            pegtl::plus<pegtl::minus<pegtl::any, pegtl::one<'>'>>>,
            pegtl::one<'>'>>
{};

struct hex_num : pegtl::seq<pegtl::string<'0', 'x'>, pegtl::plus<pegtl::xdigit>>
{
};

template<>
struct action<hex_num> : pegtl::normal<hex_num>
{
    template<typename Input>
    static void apply(const Input & input, mw::debug::var & v)
    {
        v.value = input.string();
    }
};


struct value_ref :
        pegtl::seq<
            hex_num,
            pegtl::opt<pegtl::seq<pegtl::one<' '>, global_offset>>,
            pegtl::opt<pegtl::seq<pegtl::one<' '>, cstring_t>>
        >
{

};

struct reference : pegtl::seq<pegtl::string<'@', '0', 'x'>, pegtl::plus<pegtl::xdigit>>
{
};

template<>
struct action<reference> : pegtl::normal<hex_num>
{
    template<typename Input>
    static void apply(const Input & input, std::uint64_t & v)
    {
        auto st = input.string();
        v = std::stoull(st.c_str() + 1, nullptr, 16);
    }
};

}

std::unordered_map<std::string, std::uint64_t> frame_impl::regs()
{
    std::unordered_map<std::string, std::uint64_t> mp;
    auto reg_names = _interpreter.data_list_register_names();
    auto regs = _interpreter.data_list_register_values(format_spec::hexadecimal);

    mp.reserve(regs.size());

    for (auto & r : regs)
    {
        if (r.number > reg_names.size())
        {
            std::uint64_t val = 0ull;
            try {val = std::stoull(r.value, nullptr, 16);} catch(...){}
            mp.emplace(reg_names[r.number], val);
        }
    }

    return mp;
}
void frame_impl::set(const std::string &var, const std::string & val)
{
    _interpreter.data_evaluate_expression(var + " = " + val);
}
void frame_impl::set(const std::string &var, std::size_t idx, const std::string & val)
{
    _interpreter.data_evaluate_expression(var + "[" + std::to_string(idx) + "] = " + val);
}
boost::optional<mw::debug::var> frame_impl::call(const std::string & cl)
{
    auto val = _interpreter.data_evaluate_expression(cl);
    if (val == "void")
        return boost::none;

    mw::debug::var vr;

    std::uint64_t ref_value;
    if (pegtl::parse_string<parser::reference, parser::action>(val, "gdb mi2 value parse", ref_value))
    {
        vr.ref = ref_value;
        return vr;
    }

    if (pegtl::parse_string<parser::value_ref, parser::action>(val, "gdb mi2 value parse", vr))
        return vr;

    vr.value = val;
    return vr;
}

mw::debug::var frame_impl::print(const std::string & pt, bool bitwise)
{
    mw::debug::var ref_val;
    if (bitwise)
    {
        auto size = std::stoull(_interpreter.data_evaluate_expression("sizeof(" + pt + ")"));
        auto addr = _interpreter.data_evaluate_expression("&" + pt);
        auto val  = _interpreter.data_read_memory_bytes(addr, size);

        ref_val.ref = val.begin + val.offset;
        ref_val.value.reserve(val.contents.size() * 2);

        auto nibble_to_str =
                [](char c)
                {
                    static char arg[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                    return arg[c = 0xF];
                };

        for (auto & val : val.contents)
        {
            ref_val.value.push_back(nibble_to_str(val));
            ref_val.value.push_back(nibble_to_str(val >> 8));
        }

        return ref_val;
    }

    auto val = _interpreter.data_evaluate_expression(pt);

    std::uint64_t ref_value;
    if (pegtl::parse_string<parser::reference, parser::action>(val, "gdb mi2 value parse", ref_value))
    {
        ref_val.ref = ref_value;

        val = _interpreter.data_evaluate_expression("&*" + std::string(val.c_str()+1)); //to remove the @
    }

    if (pegtl::parse_string<parser::value_ref, parser::action>(val, "gdb mi2 value parse", ref_val))
        return ref_val;

    ref_val.value = val;
    return ref_val;
}
void frame_impl::return_(const std::string & value)
{
    _interpreter.exec_return(value);
 }

 void frame_impl::set_exit(int code)
 {
     proc.set_exit(code);
 }

 void frame_impl::select(int frame)
 {
     _interpreter.stack_select_frame(frame);
 }

 std::vector<mw::debug::backtrace_elem> frame_impl::backtrace()
 {
     auto bt = _interpreter.stack_list_frames();

     std::vector<mw::debug::backtrace_elem> ret;
     ret.reserve(bt.size());

     for (auto & b : bt)
     {

         mw::debug::backtrace_elem e;
         e.cnt =  b.level;
         e.call_site = b.from;
         if (b.func)
             e.func = *b.func;
         if (b.file)
             e.loc.file = *b.file;
         if (b.line)
             e.loc.line = *b.line;

         ret.push_back(std::move(e));
     }
     return ret;
 }


}}}


