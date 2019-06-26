/** \file tasks.hpp */
#ifndef TASKS_HPP_INCLUDED
#define TASKS_HPP_INCLUDED

#include "task.hpp"

#include <map>

namespace micro {

  /**
    \class tasks
    \brief Container for extended functors
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    Template vector for functors.

    \code
    micro::tasks<int,int> ts;
    micro::tasks<std::string,std::string> ts2;

    ts.subscribe("sum2", [](int a, int b)->std::any{return a+b;});
    ts2.subscribe("concatenate2", [](std::string a, std::string b)->std::any{return a+b;});

    std::shared_future<std::any> result = ts["sum2"](15, 15); result.wait();
    std::cout << std::any_cast<int>(result.get()) << std::endl;

    std::string s1 = "hello", s2 = " world !";
    result = ts2["concatenate2"](s1, s2); result.wait();
    std::cout << std::any_cast<std::string>(result.get()) << std::endl;
    \endcode
  */
  template <typename... Ts>
  class tasks final {
  private:

    std::map<std::string, std::shared_ptr<task<Ts...>>> subscribers_;
    task<Ts...> empty_task_; // for out of range access by operator[]

  public:

    /** Creates empty tasks. */
    tasks():subscribers_(),empty_task_() {}

    /** Creates tasks by copyable constructor. \param[in] rhs tasks for copying */
    tasks(const tasks<Ts...>& rhs):tasks() { *this = rhs; }

    /** Creates tasks by movable constructor. \param[in] rhs tasks for moving */
    tasks(tasks<Ts...>&& rhs):tasks() { *this = rhs; }

    ~tasks() {}

    /** Adds task into container. \param[in] nm name of task \param[in] t function/method/lambda \param[in] hlp message help for task */
    void subscribe(const std::string& nm, const decltype(std::function<std::any(Ts...)>()) &t, const std::string& hlp = {}) {
      if (!std::empty(nm) && subscribers_.find(nm) == std::end(subscribers_) && !!t) {
        subscribers_[nm] = std::make_shared<task<Ts...>>(nm, t, hlp);
      }
    }

    /** Removes task from container. \param[in] nm name of task */
    void unsubscribe(const std::string& nm) {
      if (auto it = subscribers_.find(nm); it != std::end(subscribers_)) { subscribers_.erase(it); }
    }

    /** Removes task from container. \param[in] i index of task */
    void unsubscribe(std::size_t i) {
      if (i < subscribers_.size()) {
        for (auto it = std::begin(subscribers_); it != std::end(subscribers_); ++it) {
          if (!i--) { subscribers_.erase(it); break; }
        }
      }
    }

    /** \returns Shared future for result of called task. \param[in] nm index or name of task \param[in] args arguments for task \see task::run(Args&&... args), operator[](const std::string& nm), operator[](std::size_t i) */
    template<typename T, typename... Args>
    inline std::shared_future<std::any> operator()(const T& nm, Args&&... args) {
      return (*this)[nm](std::forward<Args>(args)...);
    }

    /** \returns Number of tasks in container */
    inline std::size_t count() const { return std::size(subscribers_); }

    /** \returns True if container has task. \param[in] nm name of task */
    inline bool has(const std::string& nm) const { return (subscribers_.find(nm) != subscribers_.end()); }

    /** \returns True if container has task. \param[in] i index of task */
    inline bool has(std::size_t i) const { return (i < std::size(subscribers_)); }

    /** Clears once-flag for all tasks in container \see task::clear_once(), task::is_once() */
    void clear_once() {
      for (auto it = std::begin(subscribers_); it != std::end(subscribers_); ++it) {
        it->second->clear_once();
      }
    }

    /** \returns Minimum Idle for all tasks in container. \see task::idle() */
    inline int idle() const {
      int ret = std::numeric_limits<int>::max(), current_idle = 0;
      for (auto it = std::cbegin(subscribers_); it != std::cend(subscribers_); ++it) {
        if ((current_idle = it->second->idle()) < ret && !(ret = current_idle)) { return ret; }
      } return ret;
    }

    /** Resets all tasks in container. \see task::reset() */
    void reset() {
      for (auto it = std::begin(subscribers_); it != std::end(subscribers_); ++it) {
        it->second->reset();
      }
    }

    /** Copyable assignment. \param[in] rhs tasks for copying */
    tasks<Ts...>& operator=(const tasks<Ts...>& rhs) {
      if (this != &rhs) { subscribers_ = rhs.subscribers_; }
      return *this;
    }

    /** Movable assignment. \param[in] rhs tasks for moving */
    tasks<Ts...>& operator=(tasks<Ts...>&& rhs) {
      if (this != &rhs) { subscribers_ = std::move(rhs.subscribers_); }
      return *this;
    }

    /** \returns Const reference to task. \param[in] nm name of task in container */
    inline task<Ts...>& operator[](const std::string& nm) const {
      if (auto it = subscribers_.find(nm); it != std::cend(subscribers_)) { return *it->second.get(); }
      else { return empty_task_; }
    }

    /** \returns Reference to task. \param[in] nm name of task in container */
    inline task<Ts...>& operator[](const std::string& nm) {
      if (auto it = subscribers_.find(nm); it != std::end(subscribers_)) { return *it->second.get(); }
      else { return empty_task_; }
    }

    /** \returns Const reference to task. \param[in] i index of task in container */
    inline task<Ts...>& operator[](std::size_t i) const {
      if (i < std::size(subscribers_)) {
        for (auto it = std::cbegin(subscribers_); it != std::cend(subscribers_); ++it) {
          if (!i--) { return *it->second.get(); }
        }
      } return empty_task_;
    }

    /** \returns Reference to task. \param[in] i index of task in container */
    inline task<Ts...>& operator[](std::size_t i) {
      if (i < std::size(subscribers_)) {
        for (auto it = std::begin(subscribers_); it != std::end(subscribers_); ++it) {
          if (!i--) { return *it->second.get(); }
        }
      } return empty_task_;
    }

  };

} // namespace micro

#endif // TASKS_HPP_INCLUDED
