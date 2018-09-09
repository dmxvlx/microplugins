/** \file plugins.hpp */
#ifndef plugins_hpp_included
#define plugins_hpp_included

#include "iplugins.hpp"
#include "shared_library.hpp"
#include "singleton.hpp"
#include <iostream> // std::cerr

/**
  \mainpage Documentation API

  # Introduction
  Microplugins is a C++ plugin framework for easy creating and using plugins.

  - It supports services for plugins and communications between plugins kernel and other plugins.

  - It uses a header-only design and makes it easy to integrate with existing projects.

  - It takes care for unloading unused plugins automatically by given time.

  # Requirements
  - Compiler with support C++17 standart (including experimental filesystem)

  - Cmake >= 2.6 (for build examples)

  - Doxygen (for build documentation)

  `This framework was tested on GNU/Linux with GCC-7.3.0`

  # Warnings
  It is not supporting loading and using plugins compiled

  by different compiler and/or linked with different libstdc++

  # Installation
  Compiling:
  > $ mkdir build && cd build && cmake .. && make

  Installation:
  > $ make install

  Or just copy folder *include/microplugins* into your project

  # License
  This library is distributed under the terms of the Boost Software License - Version 1.0

  # Examples

  Creating plugin:
  ```
  #include "iplugins.hpp"

  static std::any sum2(std::any a1, std::any a2) {
    return std::any_cast<int>(a1) + std::any_cast<int>(a2);
  }


  class plugin1 : public micro::iplugin {
  public:

    plugin1(float v, const std::string& nm):micro::iplugin(v, nm) {
      subscribe<2>("sum2", sum2);
    }

  };


  static std::shared_ptr<plugin1> instance = nullptr;

  std::shared_ptr<micro::iplugin> import_plugin() {
    return instance ? instance : (instance = std::make_shared<plugin1>(1.0f, "plugin1"));
  }
  ```

  Creating service:
  ```
  #include <microplugins/plugins.hpp>

  static std::any service(std::any a1) {
    std::shared_ptr<micro::plugins> k = std::any_cast<std::shared_ptr<micro::plugins>>(a1);

    std::shared_ptr<micro::iplugin> plugin1 = k->get_plugin("plugin1");

    if (plugin1 && plugin1->has<2>("sum2")) {
      std::shared_future<std::any> result = plugin1->run<2>("sum2", 125, 175);
      result.wait();
      std::cout << std::any_cast<int>(result.get()) << std::endl;
    }

    k->stop(); // stop if we don't need service mode
    return 0;
  }


  int main() {
    std::shared_ptr<micro::plugins> k = micro::plugins::get();
    k->subscribe<1>("service", service);

    k->run();

    while (k->is_run()) micro::sleep<micro::milliseconds>(250);

    return k->error();
  }
  ```
  You can see example of plugin, and example of service
*/

namespace micro {

  /**
    \class plugins
    \brief Management for plugins
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    Class for loading and unloading plugins. Plugins has shared pointer to it.

    \example microservice.cxx
  */
  class plugins final : public iplugins, public singleton<plugins>, public std::enable_shared_from_this<plugins> {
  private:

    std::atomic<bool> do_work_, expiry_;
    std::atomic<int> error_, max_idle_;
    std::string path_; // paths for plugins
    std::vector<
      std::tuple<
        std::string,
        std::shared_ptr<iplugin>,
        std::shared_ptr<micro::shared_library>
      >
    > plugins_;

    friend class singleton<plugins>;

    /**
      Creates plugins object

      \param[in] v version of plugins kernel
      \param[in] nm name of the plugins kernel
      \param[in] path0 paths for search plugins exploded by ':', see env $PATH

      \see singleton::get(Ts&&... args), storage::storage(float v, const std::string& nm), storage::version(), storage::major(), storage::minor(), storage::name()
    */
    plugins(float v = 1.0, const std::string& nm = "microplugins service", const std::string& path0 = "microplugins"):
    iplugins(v, nm),singleton<plugins>(),std::enable_shared_from_this<plugins>(),
    do_work_(false),expiry_(true),error_(0),max_idle_(10),path_(path0),plugins_() {}

  public:

    /** \see storage::subscribe(const std::string& nm, const T& t, const std::string& hlp) */
    using storage::subscribe;

    /** \see storage::unsubscribe(T nm) */
    using storage::unsubscribe;

    /** \see storage::run_once(T nm, Ts2&&... args) */
    using storage::run_once;

    plugins& operator=(const plugins& rhs) = delete;

    plugins& operator=(plugins&& rhs) = delete;

    ~plugins() override { stop(); }

    /** \returns Shared pointer to interface. \see iplugins::iplugins(float v, const std::string& nm), storage::storage(float v, const std::string& nm) */
    std::shared_ptr<iplugins> get_shared_ptr() override {
      return std::shared_ptr<iplugins>(shared_from_this());
    }

    /** \returns State of plugins kernel. \see run() */
    bool is_run() const { return do_work_; }

    /** \returns Error. */
    int error() const { return error_; }

    /** \returns Max idle in minutes. \see max_idle(int i) */
    int max_idle() const { return max_idle_; }

    /** Sets max idle. All loaded plugins thats has idle more or equal to it value will be unloaded. \param[in] i value in minutes, 0 - for unlimited resident in RAM. \see max_idle() */
    void max_idle(int i) { if (i >= 0) max_idle_ = i; }

    /** Runs thread for manage plugins. If plugins kernel has task with name `service' it will called once. \see is_run() */
    void run() {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if (do_work_) return;
      error_ = 0;
      do_work_ = true; expiry_ = false;
      std::thread(&plugins::loop_cb, this, shared_from_this()).detach();
      std::thread(&plugins::service_cb, this, shared_from_this()).detach();
    }

    /** Stops thread of management plugins. \see run(), is_run() */
    void stop() {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if (!do_work_) return;
      do_work_ = false;
      while (!expiry_) micro::sleep<micro::milliseconds>(1);
      unload_plugins();
      std::get<0>(tasks_)->clear_once(); std::get<1>(tasks_)->clear_once();
      std::get<2>(tasks_)->clear_once(); std::get<3>(tasks_)->clear_once();
      std::get<4>(tasks_)->clear_once(); std::get<5>(tasks_)->clear_once();
      std::get<6>(tasks_)->clear_once();
    }

    /** \returns Amount of loaded plugins in this moment. \see iplugins::count_plugins() */
    int count_plugins() const override {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      return do_work_ ? int(plugins_.size()) : 0;
    }

    /** \returns Shared pointer to plugin. \param[in] nm name of plugin \see iplugins::get_plugin(const std::string& nm) */
    std::shared_ptr<iplugin> get_plugin(const std::string& nm) override {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if (!do_work_) return nullptr;
      // search in loaded dll's
      for (std::size_t _i = 0; _i < plugins_.size(); _i++) {
        if (std::get<0>(plugins_[_i]) == nm) {
          if (std::get<1>(plugins_[_i])) return std::get<1>(plugins_[_i]);
          else plugins_.erase(plugins_.begin()+_i--);
        }
      }
      // try to load dll from system
      std::shared_ptr<micro::shared_library> dll = nullptr;
      dll = std::make_shared<micro::shared_library>(nm, path_, RTLD_GLOBAL|RTLD_LAZY);
      if (dll && dll->is_loaded()) {
        std::function<import_plugin_cb_t> lp = dll->get<import_plugin_cb_t>("import_plugin");
        std::shared_ptr<iplugin> pl = nullptr;
        if (lp && (pl = lp())) {
          pl->plugins_ = get_shared_ptr();
          plugins_.push_back({nm,pl,dll});
          std::thread(&plugins::service_plugin_cb, this, pl).detach();
          return pl;
        }
      }
      return nullptr;
    }

    /** \returns Shared pointer to plugin. \param[in] i index of plugin \see count_plugins(), iplugins::get_plugin(int i) */
    std::shared_ptr<iplugin> get_plugin(int i) override {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if (do_work_ && i >= 0 && i < int(plugins_.size())) {
        return std::get<1>(plugins_[i]);
      } else return nullptr;
    }

    /** Unloads plugin. \param[in] nm name of plugin */
    void unload_plugin(const std::string& nm) {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if (!do_work_) return;
      for (std::size_t _i = 0; _i < plugins_.size(); _i++) {
        if (std::get<0>(plugins_[_i]) == nm) {
          if (std::get<1>(plugins_[_i])->do_work_) std::get<1>(plugins_[_i])->do_work_ = false;
          while (std::get<1>(plugins_[_i]).use_count() > 1) {
            std::cerr << "wait termination plugin: " << std::get<1>(plugins_[_i])->name() << std::endl;
            micro::sleep<micro::seconds>(1);
          } plugins_.erase(plugins_.begin()+_i);
          break;
        }
      }
    }

    /** Unloads plugin. \param[in] i index of plugin \see count_plugins() */
    void unload_plugin(int i) {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if (do_work_ && i >= 0 && i < int(plugins_.size())) {
        if (std::get<1>(plugins_[i])->do_work_) std::get<1>(plugins_[i])->do_work_ = false;
        while (std::get<1>(plugins_[i]).use_count() > 1) {
          std::cerr << "wait termination plugin: " << std::get<1>(plugins_[i])->name() << std::endl;
          micro::sleep<micro::seconds>(1);
        } plugins_.erase(plugins_.begin()+i);
      }
    }

  private:

    void service_plugin_cb(std::shared_ptr<iplugin> pl) {
      if (!pl->has<1>("service")) return;
      std::shared_future<std::any> r;
      pl->do_work_ = true;
      r = (*std::get<1>(pl->tasks_).get())["service"].run_once(std::make_any<std::shared_ptr<iplugin>>(pl));
      r.wait();
    }

    void service_cb(std::shared_ptr<plugins> k) {
      if (!k->has<1>("service")) return;
      std::shared_future<std::any> r;
      r = (*std::get<1>(k->tasks_).get())["service"].run_once(std::make_any<std::shared_ptr<plugins>>(k));
      r.wait();
      if (r.valid() && r.get().has_value() && r.get().type() == typeid(int)) {
        k->error_ = std::any_cast<int>(r.get());
      } else k->error_ = -1;
    }

    void loop_cb(std::shared_ptr<plugins> k) {
      micro::clock_t last_check = micro::now();
      while (k->do_work_) {
        if (micro::duration<micro::minutes>(last_check, micro::now()) >= 1) {
          last_check = micro::now();
          if (!k->max_idle_) continue;
          // unload plugin which has idle more or equal than 10 minutes
          // and the plugin is not service (has no `service' in tasks<1>)
          std::unique_lock<std::shared_mutex> lock(k->mtx_);
          for (std::size_t _i = 0; _i < k->plugins_.size(); _i++) {
            if (std::get<1>(k->plugins_[_i])) {
              if (k->max_idle_ && std::get<1>(k->plugins_[_i])->idle() >= k->max_idle_ &&
                  !std::get<1>(k->plugins_[_i])->has<1>("service")) {
                k->plugins_.erase(k->plugins_.begin()+_i--);
              }
            } else { k->plugins_.erase(k->plugins_.begin()+_i--); }
          }
        } micro::sleep<micro::milliseconds>(250);
      } k->expiry_ = true;
    }

    void unload_plugins() {
      while (plugins_.size() > 0) {
        for (std::size_t _i = 0; _i < plugins_.size(); _i++) {
          if (std::get<1>(plugins_[_i])->do_work_) std::get<1>(plugins_[_i])->do_work_ = false;
          if (std::get<1>(plugins_[_i]).use_count() > 1) {
            std::cerr << "wait termination plugin: " << std::get<1>(plugins_[_i])->name() << std::endl;
          } else plugins_.erase(plugins_.begin()+_i--);
        } if (plugins_.size() > 0) micro::sleep<micro::seconds>(1);
      }
    }

  };

} // namespace micro

#endif // plugins_hpp_included
