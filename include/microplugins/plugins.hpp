/** \file plugins.hpp */
#ifndef PLUGINS_HPP_INCLUDED
#define PLUGINS_HPP_INCLUDED

#include "iplugins.hpp"
#include "shared_library.hpp"
#include "singleton.hpp"

#include <iostream> // std::clog

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
  > $ mkdir build && cd build && cmake -DMAX_PLUGINS_ARGS=12 ../ && make

  Installation:
  > $ make install

  Or just copy folder *include/microplugins* into your project

  # License
  This library is distributed under the terms of the Boost Software License - Version 1.0

  # Examples

  Creating plugin:
  ```
  #include <microplugins/iplugins.hpp>

  static std::any sum2(std::any a1, std::any a2) {
    return std::any_cast<int>(a1) + std::any_cast<int>(a2);
  }


  class plugin1 : public micro::iplugin<> {
  public:

    plugin1(int v, const std::string& nm):micro::iplugin<>(v, nm) {
      subscribe<2>("sum2", sum2);
    }

  };


  static std::shared_ptr<plugin1> instance = nullptr;

  std::shared_ptr<micro::iplugin<>> import_plugin() {
    return instance ? instance : (instance = std::make_shared<plugin1>(micro::make_version(1,0), "plugin1"));
  }
  ```

  Creating service:
  ```
  #include <microplugins/plugins.hpp>

  static std::any service(std::any a1) {
    std::shared_ptr<micro::plugins<>> k = std::any_cast<std::shared_ptr<micro::plugins<>>>(a1);

    std::shared_ptr<micro::iplugin<>> plugin1 = k->get_plugin("plugin1");

    if (plugin1 && plugin1->has<2>("sum2")) {
      std::shared_future<std::any> result = plugin1->run<2>("sum2", 125, 175);
      result.wait();
      std::cout << std::any_cast<int>(result.get()) << std::endl;
    }

    k->stop(); // stop if we don't need service mode
    return 0;
  }


  int main() {
    std::shared_ptr<micro::plugins<>> k = micro::plugins<>::get();
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
  template<std::size_t L = MAX_PLUGINS_ARGS>
  class plugins final : public iplugins<>, public singleton<plugins<L>>, public std::enable_shared_from_this<plugins<L>> {
  private:

    friend class singleton<plugins<L>>;

    std::atomic<bool> do_work_, expiry_;
    std::atomic<int> error_, max_idle_;
    std::string path_; // paths for plugins

    std::map<
      std::string,
      std::tuple<
        std::shared_ptr<shared_library>,
        std::shared_ptr<iplugin<>>
      >
    > plugins_;

    /**
      Creates plugins object

      \param[in] v version of plugins kernel
      \param[in] nm name of the plugins kernel
      \param[in] path0 paths for search plugins exploded by ':', see env $PATH

      \see singleton::get(Args&&... args), storage::storage(int v, const std::string& nm), storage::version(), storage::name()
    */
    explicit plugins(int v = make_version(1,0), const std::string& nm = "microplugins service", const std::string& path0 = "microplugins"):
    iplugins<>(v, nm),singleton<plugins<L>>(),std::enable_shared_from_this<plugins<L>>(),
    do_work_(false),expiry_(true),error_(0),max_idle_(10),path_(path0),plugins_() {
      static_assert(L > 0, "\n\nPlease, set up MAX_PLUGINS_ARGS constant as least to value 1 by: /path/to/build $ cmake -DMAX_PLUGINS_ARGS=12 ../ or what you need...\n");
    }

  public:

    /** \see storage::subscribe(const std::string& nm, const T& t, const std::string& hlp) */
    using storage<>::subscribe;

    /** \see storage::unsubscribe(const T& nm) */
    using storage<>::unsubscribe;

    plugins& operator=(const plugins& rhs) = delete;

    plugins& operator=(plugins&& rhs) = delete;

    ~plugins() override { stop(); }

    /** \returns Shared pointer to interface. \see iplugins::iplugins(int v, const std::string& nm), storage::storage(int v, const std::string& nm) */
    std::shared_ptr<iplugins<>> get_shared_ptr() override {
      return std::shared_ptr<iplugins<>>(plugins<>::shared_from_this());
    }

    /** \returns State of plugins kernel. \see run() */
    bool is_run() const { return do_work_; }

    /** \returns Error. */
    int error() const { return error_; }

    /** \returns Max idle in minutes. \see max_idle(int i) */
    int max_idle() const { return max_idle_; }

    /** Sets max idle. All loaded plugins thats has idle more or equal to it value will be unloaded. \param[in] i value in minutes, 0 - for unlimited resident loaded plugins in RAM. \see max_idle() */
    void max_idle(int i) { if (i >= 0) { max_idle_ = i; } }

    /** Runs thread for manage plugins. If plugins kernel has task with name `service' it will called once. \see is_run() */
    void run() {
      std::unique_lock<std::shared_mutex> lock(storage<>::mtx_);
      if (do_work_) { return; }
      error_ = 0;
      do_work_ = true;
      expiry_ = false;
      std::thread(&plugins<>::loop_cb, plugins<>::shared_from_this(), plugins<>::shared_from_this()).detach();
      std::thread(&plugins<>::service_cb, plugins<>::shared_from_this(), plugins<>::shared_from_this()).detach();
    }

    /** Stops thread of management plugins. \see run(), is_run() */
    void stop() {
      std::unique_lock<std::shared_mutex> lock(storage<>::mtx_);
      if (!do_work_) { return; }
      do_work_ = false;
      while (!expiry_) { micro::sleep<micro::milliseconds>(50); }
      unload_plugins();
      storage<>::clear_once();
    }

    /** \returns Amount of loaded plugins in this moment. \see iplugins::count_plugins() */
    std::size_t count_plugins() const override {
      std::shared_lock<std::shared_mutex> lock(storage<>::mtx_);
      return do_work_ ? std::size(plugins_) : 0;
    }

    /** \returns Shared pointer to plugin. \param[in] nm name of plugin \see iplugins::get_plugin(const std::string& nm) */
    std::shared_ptr<iplugin<>> get_plugin(const std::string& nm) override {
      std::unique_lock<std::shared_mutex> lock(storage<>::mtx_);
      if (!do_work_) { return nullptr; }
      // search in loaded dll's
      if (auto it = plugins_.find(nm); it != std::end(plugins_)) { return std::get<1>(it->second); }
      // try to load dll from system
      std::shared_ptr<iplugin<>> ret = nullptr;
      if (auto dll = std::make_shared<shared_library>(nm, path_); dll && dll->is_loaded()) {
        if (auto loader = dll->get<import_plugin_cb_t>("import_plugin"); loader && (ret = loader())) {
          if (ret->max_args() == max_args()) {
            ret->plugins_ = get_shared_ptr();
            plugins_[nm] = {dll, ret};
            std::thread(&plugins<>::service_plugin_cb, plugins<>::shared_from_this(), ret).detach();
          } else {
            std::clog << "plugin '" << nm << "' has " << ret->max_args() << " arguments for functions, expected number: " << max_args() << std::endl;
          }
        }
      } return ret;
    }

    /** \returns Shared pointer to plugin. \param[in] i index of plugin \see count_plugins(), iplugins::get_plugin(int i) */
    std::shared_ptr<iplugin<>> get_plugin(std::size_t i) override {
      std::shared_lock<std::shared_mutex> lock(storage<>::mtx_);
      if (do_work_ && i < std::size(plugins_)) {
        for (auto it = std::begin(plugins_); it != std::end(plugins_); ++it) {
          if (!i--) { return std::get<1>(it->second); }
        }
      } return nullptr;
    }

    /** Unloads plugin. \param[in] nm name of plugin */
    void unload_plugin(const std::string& nm) {
      std::unique_lock<std::shared_mutex> lock(storage<>::mtx_);
      if (!do_work_) { return; }
      if (auto it = plugins_.find(nm); it != std::end(plugins_)) {
        if (std::get<1>(it->second)->do_work_) { std::get<1>(it->second)->do_work_ = false; }
        while (std::get<1>(it->second).use_count() > 1) {
          std::clog << "wait termination plugin: " << nm << std::endl;
          micro::sleep<micro::seconds>(1);
        } plugins_.erase(it);
      }
    }

    /** Unloads plugin. \param[in] i index of plugin \see count_plugins() */
    void unload_plugin(std::size_t i) {
      std::unique_lock<std::shared_mutex> lock(storage<>::mtx_);
      if (do_work_ && i < std::size(plugins_)) {
        for (auto it = std::begin(plugins_); it != std::end(plugins_); ++it) {
          if (!i--) {
            if (std::get<1>(it->second)->do_work_) { std::get<1>(it->second)->do_work_ = false; }
            while (std::get<1>(it->second).use_count() > 1) {
              std::clog << "wait termination plugin: " << std::get<1>(it->second)->name() << std::endl;
              micro::sleep<micro::seconds>(1);
            } plugins_.erase(it);
            break;
          }
        }
      }
    }

  private:

    void service_plugin_cb(std::shared_ptr<iplugin<>> pl) {
      if (!pl->has<1>("service")) { return; }
      std::shared_future<std::any> r;
      pl->do_work_ = true;
      r = std::get<1>(pl->tasks_)["service"].run_once(std::make_any<std::shared_ptr<iplugin<>>>(pl));
      r.wait();
    }

    void service_cb(std::shared_ptr<plugins<>> k) {
      if (!k->has<1>("service")) { return; }
      std::shared_future<std::any> r;
      r = std::get<1>(k->tasks_)["service"].run_once(std::make_any<std::shared_ptr<plugins<>>>(k));
      r.wait();
      if (r.valid() && r.get().has_value() && r.get().type() == typeid(int)) {
        k->error_ = std::any_cast<int>(r.get());
      } else { k->error_ = -1; }
    }

    void loop_cb(std::shared_ptr<plugins<>> k) {
      micro::clock_t last_check = micro::now();
      while (k->do_work_) {
        if (micro::duration<micro::milliseconds>(last_check, micro::now()) >= 500) {
          last_check = micro::now();
          if (!k->max_idle_) { continue; }
          // unload plugin which has idle more or equal than `max_idle_' minutes
          // and the plugin is not service (has no task with name `service' in tasks_<1>)
          std::unique_lock<std::shared_mutex> lock(k->mtx_);
          for (auto it = std::begin(k->plugins_); it != std::end(k->plugins_); ++it) {
            if (std::get<1>(it->second)->idle() >= k->max_idle_ && !std::get<1>(it->second)->has<1>("service")) {
              k->plugins_.erase(it++);
            }
          }
        } micro::sleep<micro::milliseconds>(100);
      } k->expiry_ = true;
    }

    void unload_plugins() {
      while (!std::empty(plugins_)) {
        for (auto it = std::begin(plugins_); it != std::end(plugins_); ++it) {
          if (std::get<1>(it->second)->do_work_) { std::get<1>(it->second)->do_work_ = false; }
          if (std::get<1>(it->second).use_count() > 1) {
            std::clog << "wait termination plugin: " << std::get<1>(it->second)->name() << std::endl;
          } else { plugins_.erase(it++); }
        } if (!std::empty(plugins_)) { micro::sleep<micro::seconds>(1); }
      }
    }

  };

} // namespace micro

#endif // PLUGINS_HPP_INCLUDED
