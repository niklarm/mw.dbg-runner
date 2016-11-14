#include <boost/dll/alias.hpp>
#include <boost/system/api_config.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <mw/gdb/break_point.hpp>
#include <mw/gdb/frame.hpp>
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <mw/gdb/plugin.hpp>

#if defined(BOOST_WINDOWS_API)
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace mw::gdb;

#if defined(BOOST_POSIX_API)
#define call(Func, Args...) :: Func ( Args )
#define flag(Name) Name
#else
#define call(Func, Args...) :: _##Func ( Args )
#define flag(Name) _##Name
#endif


struct open_flags
{
    bool inited = false;

    int o_append   = 0;
    int o_creat    = 0;
    int o_excl     = 0;
    int o_noctty   = 0;
    int o_nonblock = 0;
    int o_sync     = 0;
    int o_trunc    = 0;
    int o_rdonly   = 0;
    int o_wronly   = 0;
    int o_rdwr     = 0;

    int s_irwxu = 0;
    int s_irusr = 0;
    int s_iwusr = 0;
    int s_ixusr = 0;
    int s_irwxg = 0;
    int s_irgrp = 0;
    int s_iwgrp = 0;
    int s_ixgrp = 0;
    int s_irwxo = 0;
    int s_iroth = 0;
    int s_iwoth = 0;
    int s_ixoth = 0;
    int s_isuid = 0;
    int s_isgid = 0;
    int s_isvtx = 0;

    void load(frame & fr)
    {
        o_append   = std::stoi(fr.print("O_APPEND")  .value);
        o_creat    = std::stoi(fr.print("O_CREAT")   .value);
        o_excl     = std::stoi(fr.print("O_EXCL")    .value);
        o_noctty   = std::stoi(fr.print("O_NOCTTY")  .value);
        o_nonblock = std::stoi(fr.print("O_NONBLOCK").value);
        o_sync     = std::stoi(fr.print("O_SYNC")    .value);
        o_trunc    = std::stoi(fr.print("O_TRUNC")   .value);
        o_rdonly   = std::stoi(fr.print("O_RDONLY")  .value);
        o_wronly   = std::stoi(fr.print("O_WRONLY")  .value);
        o_rdwr     = std::stoi(fr.print("O_RDWR")    .value);


        s_iwusr = std::stoi(fr.print("S_IWUSR").value);
        s_ixusr = std::stoi(fr.print("S_IXUSR").value);
        s_irwxg = std::stoi(fr.print("S_IRWXG").value);
        s_irgrp = std::stoi(fr.print("S_IRGRP").value);
        s_iwgrp = std::stoi(fr.print("S_IWGRP").value);
        s_ixgrp = std::stoi(fr.print("S_IXGRP").value);
        s_irwxo = std::stoi(fr.print("S_IRWXO").value);
        s_iroth = std::stoi(fr.print("S_IROTH").value);
        s_iwoth = std::stoi(fr.print("S_IWOTH").value);
        s_ixoth = std::stoi(fr.print("S_IXOTH").value);
        s_isuid = std::stoi(fr.print("S_ISUID").value);
        s_isgid = std::stoi(fr.print("S_ISGID").value);
        s_isvtx = std::stoi(fr.print("S_ISVTX").value);

        inited = true;
    }
    int get_flags(int in)
    {
        int out = 0;
#if defined (BOOST_WINDOWS_API)
        out |= _O_BINARY;
#endif
        if (in & o_append  ) out |= flag(O_APPEND);
        if (in & o_creat   ) out |= flag(O_CREAT);
        if (in & o_excl    ) out |= flag(O_EXCL);
        if (in & o_rdonly  ) out |= flag(O_RDONLY);
        if (in & o_wronly  ) out |= flag(O_WRONLY);
        if (in & o_rdwr    ) out |= flag(O_RDWR);
#if defined (BOOST_POSIX_API)
        if (in & o_noctty  ) out |= flag(O_NOCTTY);
        if (in & o_nonblock) out |= flag(O_NONBLOCK);
        if (in & o_sync    ) out |= flag(O_SYNC);
#endif
        if (in & o_trunc   ) out |= flag(O_TRUNC);

        return out;
    }

    int get_mode(int in)
    {
        int out = 0;
        if (in & s_irwxu) out |= flag(S_IRWXU);
        if (in & s_irusr) out |= flag(S_IRUSR);
        if (in & s_iwusr) out |= flag(S_IWUSR);
        if (in & s_ixusr) out |= flag(S_IXUSR);
#if defined (BOOST_POSIX_API)
        if (in & s_irwxg) out |= flag(S_IRWXG);
        if (in & s_irgrp) out |= flag(S_IRGRP);
        if (in & s_iwgrp) out |= flag(S_IWGRP);
        if (in & s_ixgrp) out |= flag(S_IXGRP);
        if (in & s_irwxo) out |= flag(S_IRWXO);
        if (in & s_iroth) out |= flag(S_IROTH);
        if (in & s_iwoth) out |= flag(S_IWOTH);
        if (in & s_ixoth) out |= flag(S_IXOTH);
        if (in & s_isuid) out |= flag(S_ISUID);
        if (in & s_isgid) out |= flag(S_ISGID);
        if (in & s_isvtx) out |= flag(S_ISVTX);
#endif
        return out;
    }

};

struct seek_flags
{
    bool inited = false;


    int seek_set = 0;
    int seek_cur = 0;
    int seek_end = 0;

    void load(frame & fr)
    {
        seek_set = std::stoi(fr.print("SEEK_SET").value);
        seek_cur = std::stoi(fr.print("SEEK_CUR").value);
        seek_end = std::stoi(fr.print("SEEK_END").value);
        inited = true;
    }
    int get_flags(int in)
    {
        int out = 0;

        if (in & seek_set ) out |= SEEK_SET;
        if (in & seek_cur ) out |= SEEK_CUR;
        if (in & seek_end ) out |= SEEK_END;

        return out;
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

        fr.log() << "***mw_newlib*** Log: Invoking close(" << fd << ") -> " << ret << std::endl;

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

        fr.log() << "***mw_newlib*** Log: Invoking fstat(" << fd << ", **local pointer**) -> " << ret << std::endl;


        fr.return_(std::to_string(ret));
    }

    void isatty(frame & fr)
    {
        auto fd = std::stoi(fr.arg_list().at(3).value);
        auto ret = call(isatty, fd);

        fr.log() << "***mw_newlib*** Log: Invoking isatty(" << fd << ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    void link(frame & fr)
    {
        auto existing = fr.get_cstring(1);
        auto _new     = fr.get_cstring(2);
#if defined (BOOST_POSIX_API)
        auto ret = link(existing, _new);
#else
        int ret = 0;
        if (!CreateHardLinkA(_new.c_str(), existing.c_str(), nullptr))
            ret = GetLastError();
#endif

        fr.log() << "***mw_newlib*** Log: Invoking link(" << existing << ", " << _new <<  ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    seek_flags sf;

    void lseek(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto ptr = std::stoi(fr.arg_list().at(4).value);
        auto dir_in = std::stoi(fr.arg_list().at(5).value);

        if (!sf.inited)
            sf.load(fr);

        auto dir = sf.get_flags(dir_in);
        auto ret = call(lseek,fd, ptr, dir);

        fr.log() << "***mw_newlib*** Log: Invoking lseek(" << fd << ", " << ptr << ", " << dir_in << ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    open_flags of;

    void open(frame & fr)
    {
        auto file  = fr.get_cstring(1);
        auto flags_in = std::stoi(fr.arg_list().at(3).value);
        auto mode_in  = std::stoi(fr.arg_list().at(4).value);
        boost::algorithm::replace_all(file, "\\\\", "/");

        if (!of.inited)
            of.load(fr);

        auto flags = of.get_flags(flags_in);
        auto mode  = of.get_mode (mode_in);

        auto ret = call(open, file.c_str(), flags, mode);

        fr.log() << std::oct;
        fr.log() << "***mw_newlib*** Log: Invoking open(\"" << file << "\", 0" << flags << ", 0" << mode << ") -> " << ret << std::endl;
        fr.log() << std::dec;
        fr.return_(std::to_string(ret));
    }

    void read(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto len = std::stoi(fr.arg_list().at(4).value);
        auto ptr_id = fr.arg_list().at(6).id;

        std::vector<char> buf(len, static_cast<char>(0));

        auto ret = call(read, fd, buf.data(), len);

        for (int i = 0; i<ret; i++)
            fr.set(ptr_id, i, std::to_string(buf[i]));

        fr.log() << "***mw_newlib*** Log: Invoking read(" << fd << ", ***local pointer***, " << len << ") -> " << ret << std::endl;


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

        fr.log() << "***mw_newlib*** Log: Invoking stat(\"" << file << "\", ***local pointer***) -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    void symlink(frame & fr)
    {
        auto existing = fr.get_cstring(1);
        auto _new     = fr.get_cstring(2);
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

        fr.log() << "***mw_newlib*** Log: Invoking symlink(" << existing << ", " << _new <<  ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    void unlink(frame & fr)
    {
        auto name = fr.get_cstring(6);
#if defined (BOOST_POSIX_API)
        auto ret = ::unlink(name.c_str());
#else
        int ret = 0;
        if (!DeleteFileA(name.c_str()))
            ret = GetLastError();
#endif
        fr.log() << "***mw_newlib*** Log: Invoking unlink(" << name << ") -> " << ret << std::endl;
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

        fr.log() << "***mw_newlib*** Log: Invoking write(" << fd << ", ***local pointer***, " << len << ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }
};

std::vector<std::unique_ptr<mw::gdb::break_point>> mw_gdb_setup_bps()
{
    std::vector<std::unique_ptr<mw::gdb::break_point>> vec;

    vec.push_back(std::make_unique<mw_func_stub>());
    return vec;
};


