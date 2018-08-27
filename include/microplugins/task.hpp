/** \file task.hpp */
#ifndef task_hpp_included
#define task_hpp_included

#include "time.hpp"
#include <future>
#include <functional>
#include <any>
#include <atomic>
#include <memory>
#include <cstdlib> // std::free
#include <cxxabi.h> // abi::__cxa_demangle
#include <type_traits>
#include <limits> // std::numeric_limits

namespace micro {

  /**
    \class task
    \brief Extended functor
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    Template functor with returning value type std::any and any type of arguments.

    \code
    micro::task<int,int> t2 = [](int a, int b)->std::any{return a+b;};
    std::shared_future<std::any> result = t2(10, 90); result.wait();
    std::cout << std::any_cast<int>(result.get()) << std::endl;

    t2.name("sum2");
    t2.help("function for sum two integers; returns std::any");

    if (!t2.empty()) t2.reset();
    \endcode
  */
  template<typename... Ts>
  class task final {
  private:

    clock_t clock_;
    int args_;
    std::string signature_, name_, help_;
    decltype(std::function<std::any(Ts...)>()) fn_;
    std::atomic<bool> is_once_;

  public:

    /** Creates empty task. */
    task():clock_(micro::now()),args_(sizeof...(Ts)),
    signature_(),name_(),help_(),fn_(nullptr),is_once_(false) {}

    /** Creates task. \param[in] nm name of task \param[in] t function/method/lambda \param[in] hlp message help for task \see name(), help(), run(Ts2&&... arg), run_once(Ts2&&... arg) */
    task(const std::string& nm, const decltype(std::function<std::any(Ts...)>()) &t, const std::string& hlp = {}):
    clock_(micro::now()),args_(sizeof...(Ts)),
    signature_(typeid(std::any(Ts...)).name()),name_(nm),help_(hlp),fn_(t),is_once_(false) {
      #if defined(__GNUG__)
      int status = -1;
      std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(signature_.c_str(), NULL, NULL, &status),
        std::free
      };
      if (status == 0) signature_ = res.get();
      #endif
    }

    task(const decltype(std::function<std::any(Ts...)>()) &t):task() { *this = t; }

    task(const task<Ts...>& rhs):task() { *this = rhs; }

    task(task<Ts...>&& rhs):task() { *this = rhs; }

    ~task() {}

    /** \returns Amount of arguments for task (calculatings in compile time). */
    int args() const { return args_; }

    /** \returns Signature of task (calculatings in compile time). */
    std::string signature() const { return signature_; }

    /** \returns Shared future for result task called. \param[in] arg arguments for task */
    template<typename... Ts2>
    std::shared_future<std::any> run(Ts2&&... arg) {
      if (is_once_ || !fn_) return {};
      else {
        clock_ = micro::now();
        return std::async(std::launch::async, fn_, arg...);
      }
    }

    /** \returns Shared future for result task called once. \param[in] arg arguments for task */
    template<typename... Ts2>
    std::shared_future<std::any> run_once(Ts2&&... arg) {
      if (is_once_ || !fn_) return {};
      else {
        is_once_ = true;
        clock_ = micro::now();
        return std::async(std::launch::async, fn_, arg...);
      }
    }

    /** \see run(Ts2&&... arg) */
    template<typename... Ts2>
    std::shared_future<std::any> operator()(Ts2&&... arg) { return run(arg...); }

    /** \returns True if name of task 'service' and numbers of args is 1. */
    bool is_service() const { return (args_ == 1 && name_ == "service"); }

    /** \returns True if task was called once. \see run_once(Ts2&&... arg) */
    bool is_once() const { return is_once_; }

    /** Clears once flag. \see run_once(Ts2&&... arg), is_once() */
    void clear_once() { is_once_ = false; }

    /** \returns Name of task. \see name(const std::string& nm) */
    std::string name() const { return name_; }

    /** Sets name for task. \param[in] nm name for task \see name() */
    void name(const std::string& nm) { name_ = nm; }

    /** \returns Message help for task. \see help(const std::string& nm) */
    std::string help() const { return help_; }

    /** Sets message help for task. \param[in] hlp message help \see help() */
    void help(const std::string& hlp) { help_ = hlp; }

    /** \returns Idle for task in minutes */
    int idle()  const { return micro::duration<micro::minutes>(clock_, micro::now()); }

    /** Resets pointer to function. */
    void reset() { fn_ = nullptr; }

    /** \returns True if task is nulled. */
    bool empty() const { return (fn_.target() == nullptr); }

    task<Ts...>& operator=(const decltype(std::function<std::any(Ts...)>()) &t) {
      fn_ = t;
      return *this;
    }

    task<Ts...>& operator=(const task<Ts...>& rhs) {
      if (this != &rhs) {
        clock_ = rhs.clock_;
        args_ = rhs.args_;
        signature_ = rhs.signature_;
        name_ = rhs.name_;
        help_ = rhs.help_;
        fn_ = rhs.fn_;
        is_once_ = rhs.is_once_;
      }
      return *this;
    }

    task<Ts...>& operator=(task<Ts...>&& rhs) {
      if (this != &rhs) {
        clock_ = rhs.clock_;
        args_ = rhs.args_;
        signature_ = std::move(rhs.signature_);
        name_ = std::move(rhs.name_);
        help_ = std::move(rhs.help_);
        fn_ = std::move(rhs.fn_);
        if (rhs.is_once_) is_once_ = true; // std::atomic is not movable
        else is_once_ = false;
      }
      return *this;
    }

  };

} // namespace micro

#endif // task_hpp_included
