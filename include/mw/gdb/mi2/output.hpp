/**
 * @file  mw/gdb/mi2/output.hpp
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
#ifndef MW_GDB_MI2_OUTPUT_HPP_
#define MW_GDB_MI2_OUTPUT_HPP_

#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <cstdint>

namespace mw
{
namespace gdb
{
namespace mi2
{

enum result_class
{
    done,
    running,
    connected,
    error,
    exit
};

enum async_class
{
    stopped
};

struct stream_record
{
    enum type_t
    {
        console, target, log
    } type;
    std::string content;
};

boost::optional<stream_record> parse_stream_output(const std::string & data);

struct value;

struct result
{
    std::string variable;
    std::unique_ptr<value> value_p = std::make_unique<value>(); //shared for spirit.x3
    value & value_ = *value_p;

    result() = default;
    result(const result &);
    result(result &&) = default;

    result & operator=(const result &);
    result & operator=(result &&) = default;
};

struct tuple : std::vector<result>
{
    using father_type = std::vector<result>;
    using father_type::vector;
    using father_type::operator=;
};

struct list;

struct value : boost::variant<std::string, tuple, boost::recursive_wrapper<list>>
{
    using father_type = boost::variant<std::string, tuple, boost::recursive_wrapper<list>>;

    inline value();
    template<typename T>
    inline value(T && str);

    template<typename T>
    value & operator=(T&& rhs);

};

struct list : boost::variant<std::vector<value>, std::vector<result>>
{
    using father_type = boost::variant<std::vector<value>, std::vector<result>>;
    using father_type::variant;
    using father_type::operator=;
};


//declared here because of forward-decl.
value::value() {};

template<typename T>
value::value(T&& value) : father_type(std::forward<T>(value)) {}

template<typename T>
value &value::operator=(T&& rhs) { *static_cast<father_type*>(this) = std::forward<T>(rhs); return *this;}

struct async_output
{
    enum type_t
    {
        exec,
        status,
        notify
    } type;
    async_class class_;
    std::vector<result> results;
};

struct result_output
{
    result_class class_;
    std::vector<result> results;
};

boost::optional<stream_record> parse_stream_output(const std::string & data);

boost::optional<async_output> parse_async_output(std::uint64_t token, const std::string & value); //parse with expected token
boost::optional<std::pair<boost::optional<std::uint64_t>, async_output>> parse_async_output(const std::string & value); //parse and maybe get a token
bool is_gdb_end(const std::string & data);

boost::optional<result_output> parse_record(std::uint64_t token, const std::string & data);
boost::optional<std::pair<boost::optional<std::uint64_t>, result_output>> parse_record(const std::string & data);

}
}
}

#endif /* MW_GDB_MI2_OUTPUT_HPP_ */
