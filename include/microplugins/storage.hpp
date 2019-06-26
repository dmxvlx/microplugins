/** \file storage.hpp */
#ifndef STORAGE_HPP_INCLUDED
#define STORAGE_HPP_INCLUDED

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

    You can change it for your needs by defining constant with cmake while configure:

    > ~/build $ cmake -DMAX_PLUGINS_ARGS=12 ../

    \see subscribe(const std::string& nm, const T& t, const std::string& hlp), unsubscribe(const T& nm)
  */
  template<std::size_t L = MAX_PLUGINS_ARGS>
  class storage {
  private:

    template<std::size_t> friend class plugins;

    mutable std::shared_mutex mtx_;
    int version_;
    std::string name_;

    template<typename T, std::size_t N, typename... Args>
    struct gen_tasks_type { using type = typename gen_tasks_type<T, N-1, T, Args...>::type; };

    template<typename T, typename... Args>
    struct gen_tasks_type<T, 0, Args...> { using type = tasks<Args...>; };

    template<typename T, std::size_t N, typename... Args>
    struct gen_storage_type { using type = typename gen_storage_type<T, N-1, typename gen_tasks_type<T, N>::type, Args...>::type; };

    template<typename T, typename... Args>
    struct gen_storage_type<T, 0, Args...> { using type = std::tuple<typename gen_tasks_type<T, 0>::type, Args...>; };

    typename gen_storage_type<std::any, L>::type tasks_;

  protected:

    /** Creates storage of tasks. \param[in] v version of storage \param[in] nm name of storage */
    explicit storage(int v = make_version(1,0), const std::string& nm = {}):mtx_(),version_(v),name_(nm),tasks_() {}

    /** Adds task into storage for given number arguments in I. \param[in] nm name of task \param[in] t function/method/lambda \param[in] hlp message help for task */
    template<std::size_t I, typename T>
    void subscribe(const std::string& nm, const T& t, const std::string& hlp = {}) {
      static_assert((L > 0 && I < L), "\n\nOut of range for valid number arguments of plugin's function. \nPlease, set it to larger value by: /path/to/build $ cmake -DMAX_PLUGINS_ARGS=12 ../ or what you need...\n");
      std::unique_lock<std::shared_mutex> lock(mtx_);
      std::get<I>(tasks_).subscribe(nm, t, hlp);
    }

    /** Removes task from storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    void unsubscribe(const T& nm) {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) {
        if (std::get<I>(tasks_)[nm].is_service() && std::get<I>(tasks_)[nm].is_once()) { return; }
        else { std::get<I>(tasks_).unsubscribe(nm); }
      }
    }

    /** Runs task once for given number arguments in I. \param[in] nm index or name of task \param[in] args arguments for task \returns Shared future for result \see std::shared_future, std::any, std::async */
    template<std::size_t I, typename T, typename... Args>
    inline std::shared_future<std::any> run_once(const T& nm, Args&&... args) {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_)[nm].run_once(std::forward<Args>(args)...); }
      else { return {}; }
    }

    /** Clears once flag in all tasks of this container. \see clear_once_impl(T& tasks_) */
    void clear_once() { clear_once_impl(tasks_); }

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
    std::size_t max_args() const { return L; }

    /** Runs task if it is not once-called for given number arguments in I. \param[in] nm index or name of task \param[in] args arguments for task \returns Shared future for result \see std::shared_future, std::any, std::async */
    template<std::size_t I, typename T, typename... Args>
    inline std::shared_future<std::any> run(const T& nm, Args&&... args) {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_)[nm](std::forward<Args>(args)...); }
      else { return {}; }
    }

    /** \returns Amount tasks in storage for given number arguments in I. */
    template<std::size_t I>
    inline std::size_t count() const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_).count(); }
      else { return 0; }
    }

    /** \returns True if tasks in storage has task for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    inline bool has(const T& nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_).has(nm); }
      else { return false; }
    }

    /** \returns True if tasks in storage has onced-flag for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    inline bool is_once(const T& nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_)[nm].is_once(); }
      else { return false; }
    }

    /** \returns Name of task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    std::string name(const T& nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_)[nm].name(); }
      else { return {}; }
    }

    /** \returns Message help for task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    std::string help(const T& nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_)[nm].help(); }
      else { return {}; }
    }

    /** \returns Idle(in minutes) for task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<std::size_t I, typename T>
    inline int idle(const T& nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_)[nm].idle(); }
      else { return 0; }
    }

    /** \returns Idle(in minutes) for all tasks in storage for given number arguments in I. */
    template<std::size_t I>
    inline int idle() const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I < L) { return std::get<I>(tasks_).idle(); }
      else { return 0; }
    }

    /** \returns Idle(in minutes) for all tasks in storage. */
    inline int idle() const {
      int ret = std::numeric_limits<int>::max(), current_idle = 0;
      if ((current_idle = idle<0>()) < ret && !(ret = current_idle)) { return ret; }
      if ((current_idle = idle<1>()) < ret && !(ret = current_idle)) { return ret; }
      if ((current_idle = idle<2>()) < ret && !(ret = current_idle)) { return ret; }
      if ((current_idle = idle<3>()) < ret && !(ret = current_idle)) { return ret; }
      if ((current_idle = idle<4>()) < ret && !(ret = current_idle)) { return ret; }
      if ((current_idle = idle<5>()) < ret && !(ret = current_idle)) { return ret; }
      if ((current_idle = idle<6>()) < ret && !(ret = current_idle)) { return ret; }
      return ret;
    }

  private:

    template<std::size_t I = 0, typename T>
    inline constexpr static typename std::enable_if_t<I == std::tuple_size_v<T>, void>
    clear_once_impl(T& ts) {
      if constexpr (std::tuple_size_v<T> > 0) { if (std::get<0>(ts).count()) {} }
    }

    template<std::size_t I = 0, typename T>
    inline constexpr static typename std::enable_if_t<I < std::tuple_size_v<T>, void>
    clear_once_impl(T& ts) {
      std::get<I>(ts).clear_once();
      clear_once_impl<I+1>(ts);
    }

  };

} // namespace micro

#endif // STORAGE_HPP_INCLUDED
