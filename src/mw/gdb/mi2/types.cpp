#include <mw/gdb/mi2/types.hpp>

#include <boost/fusion/algorithm/iteration/for_each.hpp>

namespace mw
{
namespace gdb
{
namespace mi2
{

template<typename T>
struct is_vector : std::false_type {};

template<typename T>
struct is_vector<std::vector<T>> : std::true_type {};

template<typename T> struct tag {};

template<typename T> constexpr inline auto reflect(const T& );


inline const value& find(const std::vector<result> & input, const char * id)
{
    auto itr = std::find_if(input.begin(), input.end(),
                            [&](const result &r){return r.variable == id;});

    if (itr == input.end())
        throw missing_value(id);

    return itr->value_;
}

inline boost::optional<const value&> find_if(const std::vector<result> & input, const char * id)
{
    auto itr = std::find_if(input.begin(), input.end(),
                            [&](const result &r){return r.variable == id;});

    if (itr == input.end())
        return boost::none;
    else
        return itr->value_;
}

template<> error_ parse_result(const std::vector<result> &r)
{
    error_ err;
    err.msg = find(r, "msg").as_string();

    if (auto code = find_if(r, "code"))
        err.code = code->as_string();

    return err;
}

template<> breakpoint parse_result(const std::vector<result> &r)
{
    breakpoint bp;

    bp.number       = std::stoi(find(r, "number").      as_string());
    bp.type         = find(r, "type").        as_string();
    bp.disp         = find(r, "disp").        as_string();
    bp.evaluated_by = find(r, "evaluated-by").as_string();
    bp.addr         = std::stoull(find(r, "addr").as_string(), 0, 16);
    bp.enabled      = find(r, "enabled").as_string() == "y";
    bp.enable       = std::stoi(find(r, "enable").as_string());
    bp.times        = std::stoi(find(r, "times"). as_string());

    if (auto cp = find_if(r, "catch-type")) bp.catch_type = cp->as_string();
    if (auto cp = find_if(r, "func")) bp.func = cp->as_string();
    if (auto cp = find_if(r, "filename")) bp.filename = cp->as_string();
    if (auto cp = find_if(r, "fullname")) bp.fullname = cp->as_string();

    if (auto cp = find_if(r, "at")) bp.at = cp->as_string();
    if (auto cp = find_if(r, "pending")) bp.pending = cp->as_string();
    if (auto cp = find_if(r, "thread")) bp.thread = cp->as_string();
    if (auto cp = find_if(r, "task")) bp.task = cp->as_string();
    if (auto cp = find_if(r, "cond")) bp.cond = cp->as_string();
    if (auto cp = find_if(r, "traceframe-usage")) bp.traceframe_usage = cp->as_string();
    if (auto cp = find_if(r, "static-tracepoint-marker-string-id")) bp.static_tracepoint_marker_string_id = cp->as_string();
    if (auto cp = find_if(r, "mask")) bp.mask = cp->as_string();
    if (auto cp = find_if(r, "original-location")) bp.original_location = cp->as_string();
    if (auto cp = find_if(r, "what")) bp.what = cp->as_string();

    if (auto cp = find_if(r, "line"))   bp.line   = std::stoi(cp->as_string());
    if (auto cp = find_if(r, "ignore")) bp.ignore = std::stoi(cp->as_string());
    if (auto cp = find_if(r, "pass"))   bp.pass   = std::stoi(cp->as_string());
    if (auto cp = find_if(r, "installed")) bp.installed = cp->as_string() == "y";

    if (auto cp = find_if(r, "thread-groups"))
    {
        const auto &l = cp->as_list().as_values();

        std::vector<std::string> th;
        th.resize(l.size());
        std::transform(l.begin(), l.end(),
                th.begin(),
                [](const value & v)
                {
                    return v.as_string();
                });

        bp.thread_groups = std::move(th);

    }

    return bp;
}


template<> watchpoint parse_result(const std::vector<result> &r)
{
    watchpoint wp;
    wp.number = std::stoi(find(r, "number").as_string());
    wp.exp = find(r, "exp").as_string();
    return wp;

}


}
}
}
