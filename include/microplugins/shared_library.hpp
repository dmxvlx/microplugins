/** \file shared_library.hpp */
#ifndef shared_library_hpp_included
#define shared_library_hpp_included

#include <functional>
#include <regex>
#include <vector>

#ifdef __has_include
#if __has_include(<filesystem>)
#include <filesystem>
namespace std_filesystem = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace std_filesystem = std::experimental::filesystem;
#else
#error "Missing <filesystem>"
#endif
#else
#include <filesystem>
namespace std_filesystem = std::filesystem;
#endif

#if defined(_WIN32) /* Is Windows ? */
#include <windows.h>
#define RTLD_LAZY     0x00001
#define RTLD_NOW      0x00002
#define RTLD_NOLOAD   0x00004
#define RTLD_GLOBAL   0x00100
#define RTLD_LOCAL    0x00000
#define RTLD_NODELETE 0x01000

/** \returns Raw pointer to loaded dll. \param[in] fl file or name of dll \param[in] m flags for loading dll */
inline void* dlopen(const char *fl, int m) {
  DWORD flags = LOAD_WITH_ALTERED_SEARCH_PATH;
  if ((m & RTLD_LAZY)) flags |= DONT_RESOLVE_DLL_REFERENCES;
  if (!fl) return static_cast<void*>(GetModuleHandle(nullptr));
  else if ((m & RTLD_NOLOAD)) return static_cast<void*>(GetModuleHandle(fl));
  else return static_cast<void*>(LoadLibraryEx(fl, nullptr, flags));
}

/** \rerurns State of unloaded dll. \param[in] hdll raw pointer to dll */
inline int dlclose(void* hdll) {
  if (!FreeLibrary(static_cast<HMODULE>(hdll))) return -1;
  else return 0;
}

/** \returns Raw pointer to symbol. \param[in] hdll raw pointer to loaded dll \param[in] s name of symbol(function name, declared as extern in the dll) */
inline void* dlsym(void* hdll, const char* s) {
  return static_cast<void*>(GetProcAddress(static_cast<HMODULE>(hdll), s));
}

#else /* Is nix ? */
#include <dlfcn.h>
#endif

namespace micro {

  /**
    \class shared_library
    \brief Manipulations for shared libraries
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    Crossplatform helper for shared libraries.
  */
  class shared_library final {
  private:

    void* dll_;
    std::string filename_;

  public:

    /** Creates empty helper dll. */
    shared_library():dll_(nullptr),filename_() {}

    shared_library(const shared_library& rhs) = delete;

    /** Creates helper dll. \param[in] name_lib name of library for loading \param[in] path0 paths for search dlls in \param[in] flags flags for loading dll \see load(const std::string& name_lib, const std::string& path0, int flags), dlopen(const char *, int) */
    shared_library(const std::string& name_lib, const std::string& path0 = {}, int flags = RTLD_GLOBAL|RTLD_LAZY):shared_library() {
      load(name_lib, path0, flags);
    }

    ~shared_library() { unload(); }

    /** \returns Full path for loaded dll. */
    std::string filename() const { return filename_; }

    /** \returns True if dll is loaded. \see load(const std::string& name_lib, const std::string& path0, int flags) */
    bool is_loaded()  const { return (dll_ != nullptr); }

    /** Unloads dll. \see load(const std::string& name_lib, const std::string& path0, int flags) */
    void unload() { if (is_loaded()) { dlclose(dll_); dll_ = nullptr; filename_.clear(); } }

    /** \returns True if dll was loaded. \param[in] name_lib name of library \param[in] path0 paths for search \param[in] flags flags for loading dll */
    bool load(const std::string& name_lib, const std::string& path0 = {}, int flags = RTLD_GLOBAL|RTLD_LAZY) {
      unload();
      return ((dll_ = load_dll(name_lib, path0, flags)) != nullptr);
    }

    /** \returns True if dll has symbol. \param[in] s name of symbol/function/variable \see dlsym(void*, const char*) */
    bool has(const std::string& s) const { return (!dll_ || !dlsym(dll_, s.c_str())) ? false : true; }

    /** \returns Pointer covered by std::function from loaded dll. \param[in] s name of function */
    template<typename T>
    std::function<T> get(const std::string& s) {
      std::function<T> r = nullptr;
      if (dll_ != nullptr) r = reinterpret_cast<T*>(dlsym(dll_, s.c_str()));
      return r;
    }

    /** \returns Raw pointer to symbol \param[in] s name of symbol \see dlsym(void*, const char*) */
    void* get_raw(const std::string& s) { return dll_ ? dlsym(dll_, s.c_str()) : nullptr; }

    shared_library& operator=(const shared_library& rhs) = delete;

  private:

    void* load_dll(const std::string& _name_lib, const std::string& path0 = {}, int flags = RTLD_GLOBAL|RTLD_LAZY) {
      void* ret = nullptr;
      std::string name_lib = _name_lib, filter_str, filter_version, env_path = ".:lib:plugins:../lib:../plugins:../lib/plugins";
      std::size_t npaths1 = 0, npaths2 = 0;
      std::vector<std::string> paths, paths0 = explode(path0, ":"), paths2 = explode(std::getenv("PATH"), ":");

      if (!path0.empty()) env_path = path0 + ":" + env_path;
      paths = explode(env_path, ":");
      npaths1 = paths.size();
      paths.insert(paths.end(), paths2.begin(), paths2.end());
      npaths2 = paths.size();

      for (std::size_t _i0 = 0; _i0 < paths0.size(); _i0++) {
        for (std::size_t _i2 = 0; _i2 < paths2.size(); _i2++) {
          std::string m = paths2[_i2] + "/";
          #ifndef _WIN32
          m += "../lib/";
          #endif
          m += paths0[_i0];
          paths.push_back(m);
        }
      }

      filter_version = "[._\\-0-9]{0,12}";
      #if defined(_WIN32) // windows
      if (name_lib.find(".dll") == std::string::npos && name_lib.find(".DLL") == std::string::npos) {
        filter_str += filter_version + ".([dDlL]{3})$";
      }
      #elif defined(__APPLE__) // macos
      if (name_lib.find("lib") != 0) name_lib = "lib" + name_lib;
      if (name_lib.find(".dylib") == std::string::npos) filter_str += filter_version + ".dylib" + filter_version;
      else filter_str += filter_version;
      #else // *nix
      if (name_lib.find("lib") != 0) name_lib = "lib" + name_lib;
      if (name_lib.find(".so") == std::string::npos) filter_str += filter_version + ".so" + filter_version;
      else filter_str += filter_version;
      #endif

      const std::regex name_lib_filter(name_lib + filter_str);
      std::error_code ec;

      for (std::size_t _i = 0; _i < paths.size(); _i++) {
        std::string str_path = paths[_i] + "/";
        #ifndef _WIN32
        if (_i >= npaths1 && _i < npaths2) str_path += "../lib/";
        #endif

        for (char* c = std::data(str_path); c && *c; c++) { if (*c == '\\') *c = '/'; }

        std_filesystem::path p(str_path);
        if (!std_filesystem::is_directory(p, ec)) continue;
        std_filesystem::directory_iterator dir_iter(p, ec), end_iter;
        for (; dir_iter != end_iter; dir_iter++) {
          if (!std_filesystem::is_regular_file(dir_iter->status())) continue;
          if (!std::regex_match(dir_iter->path().filename().generic_string(), name_lib_filter)) continue;
          if ((ret = dlopen(dir_iter->path().c_str(), flags))) {
            filename_ = dir_iter->path().generic_string();
            return ret;
          }
        }
      }

      #if defined(_WIN32)
      if (_name_lib.find("lib") != 0) return load_dll("lib" + _name_lib, path0, flags);
      #endif

      return ret;
    }

    std::vector<std::string> explode(const std::string& str, const std::string& delims) {
      std::vector<std::string> paths;
      std::size_t s = str.find_first_not_of(delims), e = 0;
      while ((e = str.find_first_of(delims, s)) != std::string::npos) {
        paths.push_back(str.substr(s, e - s));
        s = str.find_first_not_of(delims, e);
      }
      if (s != std::string::npos) paths.push_back(str.substr(s));
      return paths;
    }

  };

} // namespace micro

#endif // shared_library_hpp_included
