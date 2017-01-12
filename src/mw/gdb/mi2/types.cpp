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

template<> frame parse_result(const std::vector<result> &r)
{
    frame f;
    f.level = std::stoi(find(r, "level").as_string());
    if (auto val = find_if(r, "func")) f.func = val->as_string();
    if (auto val = find_if(r, "addr")) f.addr = std::stoull(find(r, "addr").as_string());
    if (auto val = find_if(r, "file")) f.file = val->as_string();
    if (auto val = find_if(r, "line")) f.line = std::stoi(val->as_string());
    if (auto val = find_if(r, "from")) f.from = std::stoull(val->as_string(), 0 , 16);
    if (auto val = find_if(r, "args"))
    {
        auto vec = val->as_list().as_results();
        std::vector<arg> args;
        args.resize(vec.size());
        std::transform(vec.begin(), vec.end(), args.begin(),
                    [](const result & rc) -> arg
                    {
                        return {rc.variable, rc.value_.as_string()};
                    });
        f.args = std::move(args);
    }

    return f;
}

template<> thread_info parse_result(const std::vector<result> &r)
{
    thread_info ti;
    ti.id = std::stoi(find(r, "id").as_string());
    ti.target_id = find(r, "target-id").as_string();

    ti.state = (find(r, "state").as_string() == "stopped") ?
               thread_info::stopped : thread_info::running;


    if (auto val = find_if(r, "details")) ti.details = val->as_string();
    if (auto val = find_if(r, "core"))    ti.core = std::stoi(val->as_string());
    if (auto val = find_if(r, "frame"))   ti.frame = parse_result<frame>(val->as_tuple());

    return ti;
}

template<> thread_state parse_result(const std::vector<result> & ts)
{
    std::vector<thread_info> tis;
    {
      auto ths = find(ts, "threads").as_list().as_values();
      tis.resize(ths.size());
      std::transform(ths.begin(), ths.end(), tis.begin(),
              [](const value & v)
              {
                  return parse_result<thread_info>(v.as_tuple());
              });
    }
    return {std::move(tis), std::stoi(find(ts, "current-thread-id").as_string())};
}

template<> thread_id_list parse_result(const std::vector<result> & ts)
{
    std::vector<int> tis;
    {
      auto ths = find(ts, "thread-ids").as_list().as_results();
      tis.resize(ths.size());
      std::transform(ths.begin(), ths.end(), tis.begin(),
              [](const result & r)
              {
                  if (r.variable != "thread-id")
                      throw missing_value("[" + r.variable + " != thread-id]");
                  return std::stoi(r.value_.as_string());
              });
    }
    return {std::move(tis), std::stoi(find(ts, "current-thread-id").as_string()), std::stoi(find(ts, "number-of-threads").as_string())};
}

template<> thread_select parse_result(const std::vector<result> & r)
{
    thread_select ts;

    ts.new_thread_id = std::stoi(find(r, "new-thread-id").as_string());
    if (auto val = find_if(r, "frame")) ts.frame = parse_result<frame>(val->as_tuple());
    if (auto val = find_if(r, "args"))
    {
        auto vec = val->as_list().as_results();
        std::vector<arg> args;
        args.resize(vec.size());
        std::transform(vec.begin(), vec.end(), args.begin(),
                    [](const result & rc) -> arg
                    {
                        return {rc.variable, rc.value_.as_string()};
                    });
        ts.args = std::move(args);
    }
    return ts;
}

template<> ada_task_info parse_result(const std::vector<result> & r)
{
    ada_task_info ati;

    if (auto val = find_if(r, "current")) ati.current = val->as_string();
    ati.id =      std::stoi(find(r, "id").as_string());
    ati.task_id = std::stoi(find(r, "task-id").as_string());
    if (auto val = find_if(r, "thread-id")) ati.thread_id = std::stoi(val->as_string());
    if (auto val = find_if(r, "parent-id")) ati.parent_id = std::stoi(val->as_string());
    ati.priority = std::stoi(find(r, "priority").as_string());
    ati.state = find(r, "state").as_string();
    ati.name  = find(r, "name") .as_string();

    return ati;
}

template<> varobj parse_result(const std::vector<result> & r)
{
    varobj ati;
    ati.name     = find(r, "name").as_string();
    ati.numchild = std::stoi(find(r, "numchild").as_string());
    ati.value    = find(r, "value").as_string();
    ati.type     = find(r, "type").as_string();
    if (auto val = find_if(r, "thread-id")) ati.thread_id = std::stoi(val->as_string());
    if (auto val = find_if(r, "has_more"))  ati.has_more = std::stoi(val->as_string()) > 0;

    ati.dynamic = find_if(r, "dynamic").operator bool();

    if (auto val = find_if(r, "displayhint")) ati.displayhint = val->as_string();

    if (auto val = find_if(r, "exp")) ati.exp = val->as_string();
    ati.frozen = find_if(r, "dynamic").operator bool();

    return ati;
}


template<> varobj_update parse_result(const std::vector<result> & r)
{
    varobj_update vu;
    vu.name     = find(r, "name").as_string();
    vu.value    = find(r, "value").as_string();
    if (auto val = find_if(r, "in_scope"))
    {
        const auto str = val->as_string();
        if (str == "true")
            vu.in_scope = true;
        else if (str == "false")
            vu.in_scope = false;
    }
    if (auto val = find_if(r, "type_changed"))
    {
        const auto str = val->as_string();
        if (str == "true")
            vu.type_changed = true;
        else if (str == "false")
            vu.type_changed = false;
    }
    if (auto val = find_if(r, "has_more")) vu.has_more = std::stoi(val->as_string()) > 0;
    if (auto val = find_if(r, "new_type")) vu.new_type = val->as_string();
    if (auto val = find_if(r, "new_num_children"))   vu.new_num_children = std::stoi(val->as_string());

    vu.dynamic = find_if(r, "dynamic").operator bool();

    if (auto val = find_if(r, "displayhint")) vu.displayhint = val->as_string();

     vu.dynamic      = static_cast<bool>(find_if(r, "dynamic"));
     if (auto val = find_if(r, "new_children"))
     {
         auto vals = val->as_list().as_values();
         vu.new_children.clear();
         vu.new_children.resize(vals.size());
         for (auto & v : vals)
             vu.new_children.push_back(v.as_string());
     }

    return vu;
}



}
}
}
