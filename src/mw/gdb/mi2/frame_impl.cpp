/**
 * @file   mw/gdb/mi2/frame_impl.cpp
 * @date   01.08.2016
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

#include <mw/gdb/mi2/frame_impl.hpp>
#include <mw/gdb/mi2/interpreter.hpp>
#include <algorithm>
#include <iostream>
#include <boost/algorithm/string.hpp>

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


struct char_num : pegtl::seq<pegtl::opt<pegtl::one<'-'>>, pegtl::plus<pegtl::digit>>
{
};

template<>
struct action<char_num> : pegtl::normal<char_num>
{
    template<typename Input>
    static void apply(const Input & input, std::string & v)
    {
        v = input.string();
    }
};

struct char_t :
        pegtl::seq<
            char_num,
            pegtl::one<' '>,
            pegtl::one<'\''>,
            pegtl::rep_max<4,
                pegtl::sor<
                    pegtl::string<'\\', '\''>,
                    pegtl::minus<pegtl::any, pegtl::one<'\''>>
                    >
                >,
            pegtl::one<'\''>
            >

{

};



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
    _interpreter.data_evaluate_expression('"' + var + " = " + val + '"');
}
void frame_impl::set(const std::string &var, std::size_t idx, const std::string & val)
{
    _interpreter.data_evaluate_expression('"' + var + "[" + std::to_string(idx) + "] = " + val + '"');
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

    auto is_var =
        [](const std::string & pt)
        {
            regex rx{"[^A-Za-z_]\\w*"};
            return regex_match(pt, rx);
        };

    if (bitwise && !is_var(pt))
    {
        //ok, we need to check if it's a variable first

        auto size = std::stoull(_interpreter.data_evaluate_expression("sizeof(" + pt + ")"));
        auto addr = _interpreter.data_evaluate_expression("&" + pt);
        //addr might be inside ""
        auto idx = addr.find(' ');
        if (idx != std::string::npos)
            addr = addr.substr(0, idx);

        auto vv  = _interpreter.data_read_memory_bytes(addr, size);
        auto & val = vv.at(0);

        ref_val.ref = val.begin + val.offset;
        ref_val.value.reserve(val.contents.size() * 8);

        auto bit_to_str =
                [](char c, int pos)
                {
                    return ((c >> pos) & 0b1) ? '1' : '0';
                };

        for (auto & v : boost::make_iterator_range(val.contents.rbegin(), val.contents.rend()))
        {
            ref_val.value.push_back(bit_to_str(v, 7));
            ref_val.value.push_back(bit_to_str(v, 6));
            ref_val.value.push_back(bit_to_str(v, 5));
            ref_val.value.push_back(bit_to_str(v, 4));
            ref_val.value.push_back(bit_to_str(v, 3));
            ref_val.value.push_back(bit_to_str(v, 2));
            ref_val.value.push_back(bit_to_str(v, 1));
            ref_val.value.push_back(bit_to_str(v, 0));
        }
        return ref_val;
    }

    auto val = _interpreter.data_evaluate_expression(pt);

    std::uint64_t ref_value;
    if (pegtl::parse_string<parser::reference, parser::action>(val, "gdb mi2 value parse", ref_value))
    {
        ref_val.ref = ref_value;
        val = _interpreter.data_evaluate_expression("*" + std::string(val.c_str()+1)); //to remove the @

    }

    if (pegtl::parse_string<parser::value_ref, parser::action>(val, "gdb mi2 value parse", ref_val))
    {

    }
    else
        ref_val.value = val;

    std::string char_v;
    if (pegtl::parse_string<parser::char_t, parser::action>(ref_val.value, "gdb mi2 char parse", char_v))
        ref_val.value = char_v;

    if (bitwise) //turn the int into a binary.
    {
        auto size = std::stoull(_interpreter.data_evaluate_expression("sizeof(" + pt + ")"));

        auto val = std::stoull(ref_val.value);

        std::string res;
        res.reserve(size*8);

        auto bit_to_str =
                [](std::uint64_t c, std::uint64_t pos)
                {
                    return ((c >> pos) & 0b1ull) ? '1' : '0';
                };

        bool first = true;
        auto push_back = [&](char c)
        {
            if ((c == '0') && first)
                return;
            else
            {
                first = false;
                res.push_back(c);
            }
        };

        for (auto i = size; i != 0; )
        {
            i--;
            push_back(bit_to_str(val, 7 + (i * 8)));
            push_back(bit_to_str(val, 6 + (i * 8)));
            push_back(bit_to_str(val, 5 + (i * 8)));
            push_back(bit_to_str(val, 4 + (i * 8)));
            push_back(bit_to_str(val, 3 + (i * 8)));
            push_back(bit_to_str(val, 2 + (i * 8)));
            push_back(bit_to_str(val, 1 + (i * 8)));
            push_back(bit_to_str(val, 0 + (i * 8)));
        }
        if (res.empty())
            res = "0";
        ref_val.value = res;

    }

    return ref_val;
}

mw::debug::var parse_var(interpreter & interpreter_,  const std::string & id, std::string val)
{
    mw::debug::var ref_val;
    std::uint64_t ref_value;
    if (pegtl::parse_string<parser::reference, parser::action>(val, "gdb mi2 value parse", ref_value))
    {
        ref_val.ref = ref_value;
        val = interpreter_.data_evaluate_expression("&*" + id); //to remove the @
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


