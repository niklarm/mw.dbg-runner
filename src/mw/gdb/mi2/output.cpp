/**
* @file  mw/gdb/mi2/output.cpp
 * @date   13.12.2016
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

#include <mw/gdb/mi2/output.hpp>

#include <pegtl.hh>


namespace mw
{
namespace gdb
{
namespace mi2
{

result::result(const result & rhs) : variable(rhs.variable) {value_ = rhs.value_;}

result & result::operator=(const result & rhs)
{
    variable = rhs.variable;
    value_   = rhs.value_;
    return *this;
}


namespace parser
{

template< typename Rule >
struct action
   : pegtl::nothing< Rule > {};


template<typename Rule, typename Ptr, Ptr ptr >
struct member : Rule
{
    constexpr static Ptr member_pointer = ptr;

};

// my pegtl extensions
template< typename Rule > struct control : pegtl::normal< Rule >
{
};

template<typename Rule, typename Ptr, Ptr ptr>
struct control<member<Rule, Ptr, ptr>> : pegtl::normal<Rule>
{

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, State0 && st0,  States && ... st )
    {
        return control<Rule>::template match<A, Action, Control>(in, st0.*ptr, std::forward<States>(st)...);
    }

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 && st0)
    {
        return control<Rule>::template match<A, Action, Control>(in, st0.*ptr);
    }
};

template<typename Rule>
struct push_back : Rule {};

template<typename Rule>
struct control<push_back<Rule>> : pegtl::normal<Rule>
{

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, std::vector<State0> & st0,  States && ... st )
    {
        State0 val;
        auto res = control<Rule>::template match<A, Action, Control>(in, val, std::forward<States>(st)...);
        st0.push_back(std::move(val));
        return res;
    }

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, std::vector<State0> & st0)
    {
        State0 val;
        auto res = control<Rule>::template match<A, Action, Control>(in, val);
        st0.push_back(std::move(val));
        return res;
    }
};


template<typename Rule, typename Type>
struct variant_elem : Rule {};

template<typename Rule, typename Type>
struct control<variant_elem<Rule, Type>> : pegtl::normal<Rule>
{
    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, State0 & st0,  States && ... st )
    {
        Type val;
        auto res = control<Rule>::template match<A, Action, Control>(in, val, std::forward<States>(st)...);
        st0 = std::move(val);
        return res;
    }

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 & st0)
    {
        Type val;
        auto res = control<Rule>::template match<A, Action, Control>(in, val);
        st0 = std::move(val);
        return res;
    }
};


struct done      : pegtl::string<'d', 'o', 'n', 'e'> {};
struct running   : pegtl::string<'r', 'u', 'n', 'n', 'i', 'n', 'g'> {};
struct connected : pegtl::string<'c', 'o', 'n', 'n', 'e', 'c', 't', 'e', 'd'> {};
struct error     : pegtl::string<'e', 'r', 'r', 'o', 'r'> {};
struct exit      : pegtl::string<'e', 'x', 'i', 't'> {};

template<> struct action<done>      { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::done;      } };
template<> struct action<running>   { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::running;   } };
template<> struct action<connected> { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::connected; } };
template<> struct action<error>     { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::error;     } };
template<> struct action<exit>      { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::exit;      } };

struct result_class : pegtl::sor<done, running, connected, error, exit>
{

};


struct stopped : pegtl::string<'s', 't', 'o', 'p', 'p', 'e', 'd'> {};
template<> struct action<stopped>   { template<typename T, typename ...Args> static void apply(const T&, async_class & res, Args&&...) { res = async_class::stopped;   } };


struct async_class_rule : pegtl::sor<stopped> {};

struct cstring : pegtl::seq<pegtl::one<'"'>, pegtl::star<pegtl::sor<pegtl::string<'\\', '"'>, pegtl::not_one<'"'>>>, pegtl::one<'"'>> {};


template<>
struct action<cstring>
{
   template<typename T, typename ...Args>
   static void apply( const pegtl::basic_action_input<T> & in , std::string & data, Args&&...)
   {
       if (in.size() > 2)
           data.assign(in.begin() +1, in.end() - 1);
       else
           data = "";
   }
};


struct stream_record_type : pegtl::one<'~', '@', '&'> {};

template<>
struct action<stream_record_type>
{
   template<typename T>
   static void apply( const pegtl::basic_action_input<T> & in , stream_record::type_t & data)
   {
       switch (*in.begin())
       {
       case '~':
           data = stream_record::console;
       break;
       case '@':
           data = stream_record::target;
       break;
       case '&':
           data = stream_record::log;
       break;
       }
   }
};

struct stream_output : pegtl::seq<
                        member<stream_record_type, decltype(&stream_record::type),    &stream_record::type>,
                        member<cstring,            decltype(&stream_record::content), &stream_record::content>,
                        pegtl::opt<pegtl::one<'\r'>, pegtl::eof>>
{

};


struct async_output_type : pegtl::one<'*', '+', '='> {};

template<>
struct action<async_output_type>
{
   template<typename T, typename ...Args>
   static void apply( const pegtl::basic_action_input<T> & in , async_output::type_t & data, Args&&...)
   {
       switch (*in.begin())
       {
       case '*':
           data = async_output::exec;
       break;
       case '+':
           data = async_output::status;
       break;
       case '=':
           data = async_output::status;
       break;
       }
   }
};



struct variable : pegtl::plus<pegtl::not_one<'='>> {};

template<>
struct action<variable>
{
   template<typename T>
   static void apply( const pegtl::basic_action_input<T> & in , std::string & data)
   {
       data = in.string();
   }
   template<typename T, typename ...Args>
   static void apply( const pegtl::basic_action_input<T> & in , std::string & data, Args && ...)
   {
       data = in.string();
   }
};


struct value_rule;

template<typename Rule>
struct unique_ptr : Rule {};

template<typename Rule> struct control<unique_ptr<Rule>> : pegtl::normal<Rule>
{
    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, std::unique_ptr<State0> & st0,  States && ... st )
    {
        if (!st0)
            st0 = std::make_unique<State0>();

        return control<Rule>::template match<A, Action, Control>(in, *st0, std::forward<States>(st)...);
    }
    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State>
    static bool match( Input & in, std::unique_ptr<State> & st0)
    {
        if (!st0)
            st0 = std::make_unique<State>();

        return control<Rule>::template match<A, Action, Control>(in, *st0);
    }
};


struct result_rule : pegtl::seq<
                      member<variable,      decltype(&result::variable), &result::variable>,
                      pegtl::one<'='>,
                      member<unique_ptr<value_rule>,    decltype(&result::value_p),    &result::value_p>>
{
};

struct tuple_rule : pegtl::seq<pegtl::one<'{'> , pegtl::list<push_back<result_rule>, pegtl::one<','>>, pegtl::one<'}'>> {};


struct list_rule : pegtl::sor<
                    pegtl::string<'[', ']'>,
                    pegtl::seq<pegtl::one<'['>, variant_elem<pegtl::list<push_back<result_rule>, pegtl::one<','>>, std::vector<result>>, pegtl::one<']'>>,
                    pegtl::seq<pegtl::one<'['>, variant_elem<pegtl::list<push_back<value_rule >, pegtl::one<','>>, std::vector<value >>, pegtl::one<']'>>> {};

struct value_rule : pegtl::sor<
                        variant_elem<cstring, std::string>,
                        variant_elem<tuple_rule, tuple>,
                        variant_elem<list_rule,  list>
                    > {};


struct async_output_rule : pegtl::seq<
                            member<async_output_type,  decltype(&async_output::type), &async_output::type>,
                            member<async_class_rule, decltype(&async_output::class_), &async_output::class_>,
                            member<pegtl::star<pegtl::one<','>, push_back<result_rule>>,  decltype(&async_output::results), &async_output::results>,
                            pegtl::opt<pegtl::one<'\r'>>, pegtl::eof> {};


struct result_record_rule : pegtl::seq<
                            pegtl::one<'^'>,
                            member<result_class, decltype(&result_output::class_), &result_output::class_>,
                            member<pegtl::star<pegtl::one<','>, push_back<result_rule>>, decltype(&result_output::results), &result_output::results>,
                            pegtl::opt<pegtl::one<'\r'>>, pegtl::eof> {};


struct token : pegtl::plus<pegtl::digit>
{

};


template<>
struct control<token> : pegtl::normal<token>
{
    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( pegtl::basic_memory_input<Input> & in, State0 && st0,  const std::uint64_t &tk, States && ... st )
    {
        auto beg = in.begin();
        auto res = pegtl::normal<token>::template match<A, Action, Control>(in, st0, std::forward<States>(st)...);
        if (res)
        {
            auto end = in.begin();
            auto val = std::stoull(std::string(beg, end));
            return val == tk;
        }
        return false;
    }

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 && st0, const std::uint64_t &tk)
    {
        auto beg = in.begin();
        auto res = pegtl::normal<token>::template match<A, Action, Control>(in, st0);
        if (res)
        {
            auto end = in.begin();
            auto val = std::stoull(std::string(beg, end));
            return val == tk;

        }
        return false;
    }

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( pegtl::basic_memory_input<Input> & in, State0 && st0,  boost::optional<std::uint64_t> &tk, States && ... st )
    {
        auto beg = in.begin();
        auto res = pegtl::normal<token>::template match<A, Action, Control>(in, st0, std::forward<States>(st)...);
        if (res)
        {
            auto end = in.begin();
            tk = std::stoull(std::string(beg, end));
            return true;
        }
        return false;
    }

    template<pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 && st0, boost::optional<std::uint64_t> &tk)
    {
        auto beg = in.begin();
        auto res = pegtl::normal<token>::template match<A, Action, Control>(in, st0);
        if (res)
        {
            auto end = in.begin();
            tk = std::stoull(std::string(beg, end));
            return true;

        }
        return false;
    }
};

}


boost::optional<stream_record> parse_stream_output(const std::string & data)
{
    stream_record sr;
    if (pegtl::parse_string<parser::stream_output, parser::action, parser::control>(data, "gdb-input-stream", sr))
        return sr;
    else
        return boost::none;
}

boost::optional<async_output> parse_async_output(std::uint64_t token, const std::string & data)
{
    using type = pegtl::seq<parser::token, parser::async_output_rule>;
    async_output res;

    if (pegtl::parse_string<type, parser::action, parser::control>(data, "gdb-input-stream", res, token))
        return res;
    else
        return boost::none;
}

boost::optional<std::pair<boost::optional<std::uint64_t>, async_output>> parse_async_output(const std::string & data)
{
    using type = pegtl::seq<
                     pegtl::opt<parser::token>,
                     parser::async_output_rule>;
    boost::optional<std::uint64_t> token;
    async_output res;


    if (pegtl::parse_string<type, parser::action, parser::control>(data, "gdb-input-stream", res, token))
    {
        return std::make_pair(token, res);
    }
    else
        return boost::none;
}

bool is_gdb_end(const std::string & data)
{
    using type = pegtl::seq<pegtl::string<'(', 'g', 'b', 'd', ')'>, pegtl::opt<pegtl::one<'\r'>>>;
    return pegtl::parse_string<type>(data, "gdb-input-stream");
}


boost::optional<result_output> parse_record(std::uint64_t token, const std::string & data)
{
    using type = pegtl::seq<parser::token, parser::result_record_rule>;
    result_output res;

    if (pegtl::parse_string<type, parser::action, parser::control>(data, "gdb-input-stream", res, token))
        return res;
    else
        return boost::none;
}

boost::optional<std::pair<boost::optional<std::uint64_t>, result_output>> parse_record(const std::string & data)
{
    using type = pegtl::seq<
                     pegtl::opt<parser::token>,
                     parser::result_record_rule>;
    boost::optional<std::uint64_t> token;
    result_output res;


    if (pegtl::parse_string<type, parser::action, parser::control>(data, "gdb-input-stream", res, token))
    {
        return std::make_pair(token, res);
    }
    else
        return boost::none;
}


}
}
}
