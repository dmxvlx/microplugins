/** \file storage.hpp */
#ifndef storage_hpp_included
#define storage_hpp_included

#include "tasks.hpp"
#include <shared_mutex>
#include <tuple>

namespace micro {

  /**
    \class storage
    \brief Storage for tasks
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    All plugins inherits from this class. It is conteiner for tasks.
    All tasks has returning and arguments type is std::any.
    Maximum arguments for tasks is 6, minimum is 0.

    \see subscribe(const std::string& nm, const T& t, const std::string& hlp), unsubscribe(T nm)
  */
  class storage {
  private:

    friend class plugins;

    mutable std::shared_mutex mtx_;
    float version_;
    std::string name_;

    using tasks0_t = tasks<>;
    using tasks1_t = tasks<std::any>;
    using tasks2_t = tasks<std::any,std::any>;
    using tasks3_t = tasks<std::any,std::any,std::any>;
    using tasks4_t = tasks<std::any,std::any,std::any,std::any>;
    using tasks5_t = tasks<std::any,std::any,std::any,std::any,std::any>;
    using tasks6_t = tasks<std::any,std::any,std::any,std::any,std::any,std::any>;

    std::tuple<
      std::shared_ptr<tasks0_t>,
      std::shared_ptr<tasks1_t>,
      std::shared_ptr<tasks2_t>,
      std::shared_ptr<tasks3_t>,
      std::shared_ptr<tasks4_t>,
      std::shared_ptr<tasks5_t>,
      std::shared_ptr<tasks6_t>
    > tasks_;

  protected:

    /** Creates storage of tasks. \param[in] v version of storage \param[in] nm name of storage */
    storage(float v = 1.0f, const std::string& nm = {}):mtx_(),version_(v),name_(nm),
    tasks_({
      std::make_shared<tasks0_t>(),
      std::make_shared<tasks1_t>(),
      std::make_shared<tasks2_t>(),
      std::make_shared<tasks3_t>(),
      std::make_shared<tasks4_t>(),
      std::make_shared<tasks5_t>(),
      std::make_shared<tasks6_t>()
    }) {}

    /** Adds task into storage. \param[in] nm name of task \param[in] t function/method/lambda \param[in] hlp message help for task */
    template<int I, typename T>
    void subscribe(const std::string& nm, const T& t, const std::string& hlp = {}) {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { std::get<I>(tasks_)->subscribe(nm, t, hlp); }
    }

    /** Remove task from storage. \param[in] nm index or name of task */
    template<int I, typename T>
    void unsubscribe(T nm) {
      std::unique_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) {
        if ((*std::get<I>(tasks_).get())[nm].is_service() && (*std::get<I>(tasks_).get())[nm].is_once()) {
          return;
        } else std::get<I>(tasks_)->unsubscribe(nm);
      }
    }

    /** Runs task once. \param[in] nm index or name of task \param[in] args arguments for task \returns shared future for result \see std::shared_future, std::any, std::async */
    template<int I, typename T, typename... Ts2>
    std::shared_future<std::any> run_once(T nm, Ts2&&... args) {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return (*std::get<I>(tasks_).get())[nm].run_once(args...); }
      else return {};
    }

  public:

    virtual ~storage() {}

    /** \returns Version of storage. */
    float version() const { return version_; }

    /** \returns Major version of storage (before fraction). */
    int major() const { return int(version_); }

    /** \returns Minor version of storage (before fraction). */
    int minor() const {
      std::string s = std::to_string(version_-int(version_));
      s = s.substr(s.find(".")+1);
      while (s.length() > 1 && s.back() == '0') s.pop_back(); /// \fixme
      return std::stoi(s);
    }

    /** \returns Name of storage. */
    std::string name() const { return name_; }

    /** \returns Maximum arguments for tasks of storage. */
    int max_args() const { return 6; }

    /** Runs task if it is not once-called. \param[in] nm index or name of task \param[in] args arguments for task \returns shared future for result \see std::shared_future, std::any, std::async */
    template<int I, typename T, typename... Ts2>
    std::shared_future<std::any> run(T nm, Ts2&&... args) {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return (*std::get<I>(tasks_).get())[nm](args...); }
      else return {};
    }

    /** \return Amount tasks in storage for given number arguments in I. */
    template<int I> int count() const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return std::get<I>(tasks_)->count(); }
      else return 0;
    }

    /** \return True if tasks in storage for given number arguments in I has task with index/name `nm'. \param[in] nm index or name of task */
    template<int I, typename T> bool has(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return std::get<I>(tasks_)->has(nm); }
      else return false;
    }

    /** \return True if tasks in storage for given number arguments in I has onced-flag with index/name `nm'. \param[in] nm index or name of task */
    template<int I, typename T>
    bool is_once(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return (*std::get<I>(tasks_).get())[nm].is_once(); }
      else return false;
    }

    /** \return Name of task in storage for given number arguments in I. \param[in] nm index of task */
    template<int I>
    std::string name(int i) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return (*std::get<I>(tasks_).get())[i].name(); }
      else return {};
    }

    /** \return Message help for task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<int I, typename T>
    std::string help(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return (*std::get<I>(tasks_).get())[nm].help(); }
      else return {};
    }

    /** \return Idle(in minutes) for task in storage for given number arguments in I. \param[in] nm index or name of task */
    template<int I, typename T>
    int idle(T nm) const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return (*std::get<I>(tasks_).get())[nm].idle(); }
      else return 0;
    }

    /** \return Idle(in minutes) for all tasks in storage for given number arguments in I. */
    template<int I>
    int idle() const {
      std::shared_lock<std::shared_mutex> lock(mtx_);
      if constexpr (I >= 0 && I <= 6) { return std::get<I>(tasks_)->idle(); }
      else return 0;
    }

    /** \return Idle(in minutes) for all tasks in storage. */
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
