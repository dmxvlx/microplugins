/** \file storage.hpp */
#ifndef storage_hpp_included
#define storage_hpp_included

#include "tasks.hpp"
#include <shared_mutex>
#include <tuple>

namespace micro {

  inline int is_le() { static const std::uint32_t i = 0x04030201; return (*((std::uint8_t*)(&i)) == 1); }
  inline int make_version(int Major, int Minor) { return ((Major << 8) | Minor); }
  inline int get_major(int Version) { return (Version >> 8); }
  inline int get_minor(int Version) { return (is_le() ? Version & 0xff : Version & 0xff000000); }

  /**
    \class storage
    \brief Storage for tasks
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    All plugins inherits from this class. It is container for tasks.
    All tasks has returning and arguments type is std::any.
    Maximum arguments for tasks is 6, minimum is 0.

    \see subscribe(const std::string& nm, const T& t, const std::string& hlp), unsubscribe(T nm)
  */
  class storage {
  private:

    friend class plugins;

    mutable std::shared_mutex mtx_;
    int version_;
    std::string name_;

    using tasks0_t = tasks<>;
    using tasks1_t = tasks<std::any>;
    using tasks2_t = tasks<std::any,std::any>;
    using tasks3_t = tasks<std::any,std::any,std::any>;
    using tasks4_t = tasks<std::any,std::any,std::any,std::any>;
    using tasks5_t = tasks<std::any,std::any,std::any,std::any,std::any>;
    using tasks6_t = tasks<std::any,std::any,std::any,std::any,std::any,std::any>;

    std::tuple<tasks0_t,tasks1_t,tasks2_t,tasks3_t,tasks4_t,tasks5_t,tasks6_t> tasks_;

  protected:

    /** Creates storage of tasks. \param[in] v version of storage \param[in] nm name of storage */
    explicit storage(int v = make_version(1,0), const std::string& nm = {}):mtx_(),version_(v),name_(nm),
    tasks_({tasks0_t(),tasks1_t(),tasks2_t(),tasks3_t(),tasks4_t(),tasks5_t(),tasks6_t()}) {}

    /** Adds task into storage for given number arguments in I. \param[in] nm name of task \param[in] t function/method/lambda \param[in] hlp message help for task */
    template<std::size_t I, typename T>
    void subscribe(const std::string& nm, const T& t, const std::string& hlp = {}) {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        std::get<I>(tasks_).subscribe(nm, t, hlp);
      }
    }

    /** Removes task from storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    void unsubscribe(T nm) {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        if (std::get<I>(tasks_)[nm].is_service() && std::get<I>(tasks_)[nm].is_once()) {
          return;
        } else std::get<I>(tasks_).unsubscribe(nm);
      }
    }

    /** Runs task once for given number arguments in I. \param[in] nm index or name of task \param[in] args arguments for task \returns Shared future for result \see std::shared_future, std::any, std::async */
    template<std::size_t I, typename T, typename... Args>
    std::shared_future<std::any> run_once(T nm, Args&&... args) {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_)[nm].run_once(args...);
      } else return {};
    }

  public:

    virtual ~storage() {}

    /** \returns Version of storage. */
    int version() const { return version_; }

    /** \returns Major version of storage. */
    int major() const { return get_major(version_); }

    /** \returns Minor version of storage. */
    int minor() const { return get_minor(version_); }

    /** \returns Name of storage. */
    const std::string& name() const { return name_; }

    /** \returns Maximum arguments for tasks of storage. */
    int max_args() const { return 6; }

    /** Runs task if it is not once-called for given number arguments in I. \param[in] nm index or name of task \param[in] args arguments for task \returns Shared future for result \see std::shared_future, std::any, std::async */
    template<std::size_t I, typename T, typename... Args>
    std::shared_future<std::any> run(T nm, Args&&... args) {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_)[nm](args...);
      } else return {};
    }

    /** \returns Amount tasks in storage for given number arguments in I. */
    template<std::size_t I>
    int count() const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_).count();
      } else return 0;
    }

    /** \returns True if tasks in storage has task for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    bool has(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_).has(nm);
      } else return false;
    }

    /** \returns True if tasks in storage has onced-flag for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    bool is_once(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_)[nm].is_once();
      } else return false;
    }

    /** \returns Name of task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    std::string name(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_)[nm].name();
      } else return {};
    }

    /** \returns Message help for task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    std::string help(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_)[nm].help();
      } else return {};
    }

    /** \returns Idle(in minutes) for task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    int idle(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_)[nm].idle();
      } else return 0;
    }

    /** \returns Idle(in minutes) for all tasks in storage for given number arguments in I. */
    template<std::size_t I>
    int idle() const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < std::tuple_size_v<std::decay_t<decltype(tasks_)>>) {
        return std::get<I>(tasks_).idle();
      } else return 0;
    }

    /** \returns Idle(in minutes) for all tasks in storage. */
    int idle() const {
      int ret = std::numeric_limits<int>::max(), current_idle = 0;
      if ((current_idle = idle<0>()) < ret && !(ret = current_idle)) return ret;
      if ((current_idle = idle<1>()) < ret && !(ret = current_idle)) return ret;
      if ((current_idle = idle<2>()) < ret && !(ret = current_idle)) return ret;
      if ((current_idle = idle<3>()) < ret && !(ret = current_idle)) return ret;
      if ((current_idle = idle<4>()) < ret && !(ret = current_idle)) return ret;
      if ((current_idle = idle<5>()) < ret && !(ret = current_idle)) return ret;
      if ((current_idle = idle<6>()) < ret && !(ret = current_idle)) return ret;
      return ret;
    }

  };

} // namespace micro

#endif // storage_hpp_included
