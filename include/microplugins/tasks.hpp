/** \file tasks.hpp */
#ifndef tasks_hpp_included
#define tasks_hpp_included

#include "task.hpp"
#include <unordered_map>

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

    using map_t = std::unordered_map<std::string, std::shared_ptr<task<Ts...>>>;
    using iterator_t = typename std::unordered_map<std::string, std::shared_ptr<task<Ts...>>>::iterator;
    using const_iterator_t = typename std::unordered_map<std::string, std::shared_ptr<task<Ts...>>>::const_iterator;

    map_t subscribers_;
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
      if (nm.empty() || subscribers_.find(nm) != subscribers_.end() || !t) return;
      subscribers_[nm] = std::make_shared<task<Ts...>>(nm, t, hlp);
    }

    /** Removes task from container. \param[in] nm name of task */
    void unsubscribe(const std::string& nm) {
      iterator_t it = subscribers_.find(nm);
      if (it != subscribers_.end()) subscribers_.erase(it);
    }

    /** Removes task from container. \param[in] i index of task */
    void unsubscribe(int i) {
      if (i >= 0 && i < int(subscribers_.size())) {
        for (iterator_t it = subscribers_.begin(); it != subscribers_.end(); it++) {
          if (!i--) { subscribers_.erase(it); break; }
        }
      }
    }

    /** \returns Shared future for result of called task. \param[in] nm index or name of task \param[in] arg arguments for task \see task::run(Ts2&&... arg), operator[](const std::string& nm), operator[](int i) */
    template<typename T, typename... Ts2>
    std::shared_future<std::any> operator()(T nm, Ts2&&... arg) {
      return (*this)[nm](arg...);
    }

    /** \returns Number of tasks in container */
    int count() const {
      return int(subscribers_.size());
    }

    /** \returns True if container has task. \param[in] nm name of task */
    bool has(const std::string& nm) const {
      return (subscribers_.find(nm) != subscribers_.end());
    }

    /** \returns True if container has task. \param[in] i index of task */
    bool has(int i) const {
      return (i >= 0 && i < int(subscribers_.size()));
    }

    /** Clears once-flag for all tasks in container \see task::clear_once(), task::is_once() */
    void clear_once() {
      for (iterator_t it = subscribers_.begin(); it != subscribers_.end(); it++) {
        it->second->clear_once();
      }
    }

    /** \returns Minimum Idle for all tasks in container. \see task::idle() */
    int idle() const {
      int ret = std::numeric_limits<int>::max(), current_idle = 0;
      for (const_iterator_t it = subscribers_.cbegin(); it != subscribers_.cend(); it++) {
        if ((current_idle = it->second->idle()) < ret && !(ret = current_idle)) return ret;
      } return ret;
    }

    /** Resets all tasks in container. \see task::reset() */
    void reset() {
      for (iterator_t it = subscribers_.begin(); it != subscribers_.end(); it++) {
        it->second->reset();
      }
    }

    /** Copyable assignment. \param[in] rhs tasks for copying */
    tasks<Ts...>& operator=(const tasks<Ts...>& rhs) {
      if (this != &rhs) subscribers_ = rhs.subscribers_;
      return *this;
    }

    /** Movable assignment. \param[in] rhs tasks for moving */
    tasks<Ts...>& operator=(tasks<Ts...>&& rhs) {
      if (this != &rhs) subscribers_ = std::move(rhs.subscribers_);
      return *this;
    }

    /** \returns Const reference to task. \param[in] nm name of task in container */
    task<Ts...>& operator[](const std::string& nm) const {
      const_iterator_t it = subscribers_.find(nm);
      if (it != subscribers_.cend()) return *it->second.get();
      else { return empty_task_; }
    }

    /** \returns Reference to task. \param[in] nm name of task in container */
    task<Ts...>& operator[](const std::string& nm) {
      iterator_t it = subscribers_.find(nm);
      if (it != subscribers_.end()) return *it->second.get();
      else { return empty_task_; }
    }

    /** \returns Const reference to task. \param[in] i index of task in container */
    task<Ts...>& operator[](int i) const {
      if (i >= 0 && i < int(subscribers_.size())) {
        for (const_iterator_t it = subscribers_.cbegin(); it != subscribers_.cend(); it++) {
          if (!i--) return *it->second.get();
        }
      } else { return empty_task_; }
    }

    /** \returns Reference to task. \param[in] i index of task in container */
    task<Ts...>& operator[](int i) {
      if (i >= 0 && i < int(subscribers_.size())) {
        for (iterator_t it = subscribers_.begin(); it != subscribers_.end(); it++) {
          if (!i--) return *it->second.get();
        }
      } else { return empty_task_; }
    }

  };

} // namespace micro

#endif // tasks_hpp_included
