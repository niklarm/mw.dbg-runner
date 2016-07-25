# Gdb-Runner

It's a tool for running binaries via gdb, local or via remote. The main purpose is to execute *tests on embedded systems*.

The features are
    
   - plugin system for custom breakpoints
   - launch services via the runner
   - debug mode (output of the whole gdb log)

# Build

This tool requires [boost](www.boost.org) and [boost-process](https://github.com/klemens-morgenstern/boost-process). Since boost process is not yet part of boost, you will need to checkout the develop branch of boost and then put the develop version of process into `boost/libs/process`.
Then you need to add the location into your `user-config.jam` as a variable `boost-loc`:

    boost-loc = F:/boost ;
    
# Usage

In order to use the gdb-runner you need a plugin. We currently provide one for the newlib, but it quite easy to write your own. But for now, we assume we have a plugin, so let's take a look into an example: 

```
mw-gdb-runner --gdb=arm-none-eabi-gdb --exe=../../arm-test/Debug/arm-test.elf --log my-log.txt --lib libmw-syscalls.dll --remote localhost:3333 --other "openocd -f ..\scripts\interface\stlink-v2.cfg -f ..\scripts\target\stm32f4x.cfg -c init"
```

Now what we do is, we run an elf-binary on a stm32f4 via `openocd` and use the *syscalls.dll* as a plugin for the breakpoints.


| Option | Description | Default |
|--------|-------------|---------|
| `--gdb` | Command for the gdb | `gdb`  |
| `--gdb-args` | Arguments to pass to gdb | _none_ |
| `--exe` | The binary to run   | _none_ |
| `--args` | Args to pass to the process *not yet supported for embedded systems* | _none_ |
| `--log` | Generate the logfile | _none_ |
| `--lib` | Breakpoint plugins | _none_ |
| `--remote` | Remote address of gdb-server | _none_ |
| `--other` | Other processes to launch, i.e. gdb-server | _none_ |
| `--debug` | Enable the debug log, i.e. put the whole gdb-communication into the console | _disables_ |
| `--timeout` | Communication Timeout [s] | `10` |

# MW Newlib syscalls

We currently provide on plugin to use with newlib for embedded systems, which uses our [systemcall-stubs](example/mw_newlib_syscalls.c). This stubs the following functions:

 - `int _fstat(int fildes, struct stat* st)`
 - `int _isatty(int file)`
 - `int _link(char* existing, char* _new)`
 - `int _open(char* file, int flags, int mode)`
 - `int _close(int fildes)`
 - `int _lseek(int file, int ptr, int dir)`
 - `int _read(int file, char* ptr, int len)`
 - `int _write(int file, char* ptr, int len)`
 - `int _stat(const char* file, struct stat* st)`
 - `int _symlink(const char* path1, const char* path2)`
 - `int _unlink(char* name)`

This allows the usage of file and console access on the embedded systems, which also allows the usage of `gcov`.

# Implementing a custom plugin

To implement a custom plugin, you need a dynamic-loaded library, which needs a entry point of the following form:

```cpp
extern "C" BOOST_SYMBOL_EXPORT std::vector<std::unique_ptr<mw::gdb::break_point>> mw_gdb_setup_bps();
```

## Breakpoint interface

```cpp
namespace mw {
namespace gdb {
class break_point
{
public:
    const std::string& identifier() const {return _identifier;}
    break_point(const std::string & func_name) : _identifier(func_name) {}
    break_point(const std::string & file_name, std::size_t line)
            : _identifier(file_name + ":" + std::to_string(line)) {}
    virtual void invoke(frame & fr, const std::string & file, int line) = 0;
    virtual void set_at(std::uint64_t addr, const std::string & file, int line) {}
    virtual void set_multiple(std::uint64_t addr, std::string & name, int line) {}
    virtual void set_not_found() {};
};
} 
}
```

### identifier

This is the identifier of the breakpoint as set in the constructor. There are two variants to identify a breakpoint:

 - function signature
 - location in file

If you use the function signature, you can either just use the function name (i.e. `printf`) which uses all overloads. Alternatively you can just take one function overload via the signature which has to exclude the names of the parameters (i.e. `to_string( int )`).

### invoke

This functino will be called when the program halts at the breakpoint. See the frame description for details on this.

### set_at

This function will be called when the breakpoint is set at a single location.

### set_multiple

This function will be called when the breakpoint is set at several locations. `gdb` does not provide the locations in this case.

### set_not_found

Called when the breakpoint wasn't found in the binary.

## frame

The frame is the type passed to the breakpoint when the program stops at the breakpoints.

It has the following interface:

```cpp
struct frame
{
    const std::vector<arg> &arg_list() const {return _arg_list;}
    const arg &arg_list(std::size_t index) const {return _arg_list.at(index);}
    inline std::string get_cstring(std::size_t index);
    virtual std::unordered_map<std::string, std::uint64_t> regs() = 0;
    virtual void set(const std::string &var, const std::string & val)   = 0;
    virtual void set(const std::string &var, std::size_t idx, const std::string & val) = 0;
    virtual boost::optional<var> call(const std::string & cl)        = 0;
    virtual var print(const std::string & pt)       = 0;
    virtual void return_(const std::string & value) = 0;
    virtual void set_exit(int) = 0;
    virtual std::ostream & log() = 0;
};
```

### arg_list()

Get the complete argument-list.

### arg_list(size_t)

Get one argument of the argument list. Will throw if out of index.

### get_cstring(size_t)

Get the cstring on the specific location. Gdb may print out the value of the cstring if one is passed so this function allows convenient access.

### regs()

Read the registers of the current frame.

### set(string, string)

Set a single variable.

### set(string, size_t, string)

Set a value in an array at the index.

### call(string)

Call the passed command. May return a value if the function is not void.

### print(string)

Print a specific value. For an array member append `[N]`.

### return_(string)

Return from the function with the given value. With a `void` function just pass an empty string.

### set_exit(int)

Set the exit value program and cause it to exit. The gdb-runner will call `quit` afterwards. This is used with `_exit` on embedded systems.

### log()

Obtain a reference to the log output of the gdb-runner.

## arg

The arg structure gives the value of a single argument passed to a function.

```cpp
struct var
{
    boost::optional<std::uint64_t> ref;
    std::string value;
    cstring_t   cstring;
};
```

### ref

The value of a reference, if such is passed.

### value

The actual value of the argument.

### cstring

The cstring value given if a string is passed. This is not a `std::string` but a value of `mw::gdb::cstring_t`. This has the following layout:

```cpp
struct cstring_t
{
    std::string value;
    bool ellipsis;
};
```

The value is the actual string part of the value. Gdb may however shorten the string and append ellipsis. If this is the case the boolean value will be set, and you will need to read the rest manually, by array access. To do this, the `frame::get_cstring` method is provided.


## var

The struct used for a variable. It inherits arg and adds the `id` (as an `std::string`), i.e. the variable name.