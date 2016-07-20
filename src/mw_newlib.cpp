#include <boost/dll/alias.hpp>
#include <boost/system/api_config.hpp>
#include <mw/gdb/break_point.hpp>
#include <mw/gdb/frame.hpp>
#include <vector>
#include <memory>
#include <io.h>
#include <sys/Stat.h>
#include <sys/types.h>

#if defined(BOOST_WINDOWS_API)
#include <windows.h>
#endif

using namespace mw::gdb;

#if defined(BOOST_POSIX_API)
#define call(Func, Args...) :: Func ( Args )
#else
#define call(Func, Args...) :: _##Func ( Args )
#endif


struct exit_stub : break_point
{
    exit_stub() : break_point("_exit(int)")
    {
    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        fr.set_exit(std::stoi(fr.arg_list().at(0).value));
    }
};

struct mw_func_stub : break_point
{
    mw_func_stub() : break_point("mw_func_stub")
    {
    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        auto type = fr.arg_list().at(0);
        if (type.id != "func_type")
            return;

        if (type.value == "mw_func_close")
            this->close(fr);
        else if (type.value == "mw_func_fstat")
            this->fstat(fr);
        else if (type.value == "mw_func_isatty")
            this->isatty(fr);
        else if (type.value == "mw_func_link")
            this->link(fr);
        else if (type.value == "mw_func_lseek")
            this->lseek(fr);
        else if (type.value == "mw_func_open")
            this->open(fr);
        else if (type.value == "mw_func_read")
            this->read(fr);
        else if (type.value == "mw_func_stat")
            this->stat(fr);
        else if (type.value == "mw_func_symlink")
            this->symlink(fr);
        else if (type.value == "mw_func_unlink")
            this->unlink(fr);
        else if (type.value == "mw_func_write")
            this->write(fr);
    }
    void close(frame & fr)
    {
        auto fd = std::stoi(fr.arg_list().at(3).value);
        auto ret = call(close, fd);
        fr.return_(std::to_string(ret));
    }
    void fstat(frame & fr)
    {
        auto fd = std::stoi(fr.arg_list().at(3).value);
#if defined (BOOST_WINDOWS_API)
        struct _stat64i32 st;
#else
        struct stat st;
#endif
        int ret = call(fstat, fd, &st);

        fr.set("arg7->st_dev",   std::to_string(st.st_dev));
        fr.set("arg7->st_ino",   std::to_string(st.st_ino));
        fr.set("arg7->st_mode",  std::to_string(st.st_mode));
        fr.set("arg7->st_nlink", std::to_string(st.st_nlink));
        fr.set("arg7->st_uid",   std::to_string(st.st_uid));
        fr.set("arg7->st_gid",   std::to_string(st.st_gid));
        fr.set("arg7->st_rdev",  std::to_string(st.st_rdev));
        fr.set("arg7->st_size",  std::to_string(st.st_size));
        fr.set("arg7->st_atime", std::to_string(st.st_atime));
        fr.set("arg7->st_mtime", std::to_string(st.st_mtime));
        fr.set("arg7->st_ctime", std::to_string(st.st_ctime));

        fr.return_(std::to_string(ret));
    }

    void isatty(frame & fr)
    {
        auto fd = std::stoi(fr.arg_list().at(3).value);
        auto ret = call(isatty, fd);
        fr.return_(std::to_string(ret));
    }

    void link(frame & fr)
    {
        auto existing = fr.arg_list().at(1).cstring;
        auto _new = fr.arg_list().at(2).cstring;
#if defined (BOOST_POSIX_API)
        auto ret = link(existing, _new);
#else
        int ret = 0;
        if (!CreateHardLinkA(_new.c_str(), existing.c_str(), nullptr))
            ret = GetLastError();
#endif
        fr.return_(std::to_string(ret));
    }

    void lseek(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto ptr = std::stoi(fr.arg_list().at(4).value);
        auto dir = std::stoi(fr.arg_list().at(5).value);

        auto ret = call(lseek,fd, ptr, dir);

        fr.return_(std::to_string(ret));
    }

    void open(frame & fr)
    {
        auto file  = fr.arg_list().at(1).cstring;
        auto flags = std::stoi(fr.arg_list().at(3).value);
        auto mode  = std::stoi(fr.arg_list().at(4).value);

        auto ret = call(open, file.c_str(), flags, mode);

        fr.return_(std::to_string(ret));
    }

    void read(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto len = std::stoi(fr.arg_list().at(4).value);
        auto ptr_id = fr.arg_list().at(6).id;

        std::vector<char> buf(len, static_cast<char>(0));

        auto ret = call(read, fd, buf.data(), len);

        for (int i = 0; i<len; i++)
            fr.set(ptr_id, i, std::to_string(buf[i]));

        fr.return_(std::to_string(ret));
    }

    void stat(frame & fr)
    {
        auto file = fr.arg_list().at(1).value;
#if defined (BOOST_WINDOWS_API)
        struct _stat64i32 st;
#else
        struct stat st;
#endif

        int ret = call(stat, file.c_str(), &st);

        fr.set("arg7->st_dev",   std::to_string(st.st_dev));
        fr.set("arg7->st_ino",   std::to_string(st.st_ino));
        fr.set("arg7->st_mode",  std::to_string(st.st_mode));
        fr.set("arg7->st_nlink", std::to_string(st.st_nlink));
        fr.set("arg7->st_uid",   std::to_string(st.st_uid));
        fr.set("arg7->st_gid",   std::to_string(st.st_gid));
        fr.set("arg7->st_rdev",  std::to_string(st.st_rdev));
        fr.set("arg7->st_size",  std::to_string(st.st_size));
        fr.set("arg7->st_atime", std::to_string(st.st_atime));
        fr.set("arg7->st_mtime", std::to_string(st.st_mtime));
        fr.set("arg7->st_ctime", std::to_string(st.st_ctime));

        fr.return_(std::to_string(ret));
    }

    void symlink(frame & fr)
    {
        auto existing = fr.arg_list().at(1).cstring;
        auto _new = fr.arg_list().at(2).cstring;
#if defined (BOOST_POSIX_API)
        auto ret = ::symlink(existing.c_str(), _new.c_str());
#else
        int ret = 0;
#if defined(CreateSymbolicLink)
        //BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_VISTA
        if (!CreateSymbolicLinkA(_new.c_str(), existing.c_str(), 0x0))
            ret = GetLastError();
#else
        if (!CreateHardLinkA(_new.c_str(), existing.c_str(), 0x0))
            ret = GetLastError();
#endif
#endif
        fr.return_(std::to_string(ret));
    }

    void unlink(frame & fr)
    {
        auto name = fr.arg_list().at(6).cstring;
#if defined (BOOST_POSIX_API)
        auto ret = ::unlink(name.c_str());
#else
        int ret = 0;
        if (!DeleteFileA(name.c_str()));
            ret = GetLastError();
#endif
        fr.return_(std::to_string(ret));
    }

    void write(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto len = std::stoi(fr.arg_list().at(4).value);
        auto ptr_id = fr.arg_list().at(6).id;
        std::vector<char> data(len, static_cast<char>(0));

        for (int i = 0u; i<len; i++)
        {
            auto val = fr.print(ptr_id + '[' + std::to_string(i) + ']');
            data[i] = std::stoi(val.value);
        }

        auto ret = call(write, fd, data.data(), len);

        fr.return_(std::to_string(ret));
    }
};

extern "C" BOOST_SYMBOL_EXPORT std::vector<std::unique_ptr<mw::gdb::break_point>> mw_gdb_setup_bps();

std::vector<std::unique_ptr<mw::gdb::break_point>> mw_gdb_setup_bps()
{
    std::vector<std::unique_ptr<mw::gdb::break_point>> vec;

    vec.push_back(std::make_unique<exit_stub>());
    vec.push_back(std::make_unique<mw_func_stub>());
    return vec;
};


