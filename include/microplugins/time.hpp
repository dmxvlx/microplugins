/** \file time.hpp */
#ifndef time_hpp_included
#define time_hpp_included

#include <thread>
#include <chrono>
#include <string>
#include <ctime>

namespace micro {

  /** \returns String with formated time. \param[in] t time \param[in] is_local local/UTC \param[in] fmt fromatting for output string */
  inline std::string get_time(std::time_t t/*std::time(nullptr)*/, bool is_local, const std::string& fmt/*"%d.%m.%Y %H:%M:%S"*/) {
    char mbstr[256];
    std::strftime(mbstr, sizeof(mbstr), fmt.c_str(), is_local ? std::localtime(&t) : std::gmtime(&t));
    return mbstr;
  }

  /** Declaration types for time functions. */
  using clock_t = std::chrono::time_point<std::chrono::system_clock>; ///< Alias for type clock
  using nanoseconds = std::chrono::nanoseconds; ///< Alias for nanoseconds
  using microseconds = std::chrono::microseconds; ///< Alias for microseconds
  using milliseconds = std::chrono::milliseconds; ///< Alias for milliseconds
  using seconds = std::chrono::seconds; ///< Alias for seconds
  using minutes = std::chrono::minutes; ///< Alias for minutes
  using hours = std::chrono::hours; ///< Alias for hours

  /** \returns System clock for current time */
  inline clock_t now() { return std::chrono::system_clock::now(); }

  /** \returns Time from system clock \param[in] t system clock */
  inline std::time_t to_time_t(clock_t t) { return std::chrono::system_clock::to_time_t(t); }

  /** \returns System clock from time \param[in] t time */
  inline clock_t from_time_t(std::time_t t) { return std::chrono::system_clock::from_time_t(t); }

  /** Sleeps for given precision T. \param[in] value value for sleep */
  template<typename T = milliseconds>
  inline void sleep(int value) { std::this_thread::sleep_for(T(value)); }

  /** \returns Duration for given period with precision T. \param[in] start started time \param[in] end ended time */
  template<typename T = milliseconds>
  inline int duration(clock_t start, clock_t end) {
    return int(std::chrono::duration_cast<T>(end-start).count());
  }

  /**
    \class stopwatch
    \brief Stopwatch for measure time
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    \code
    micro::stopwatch timer;

    // do something here
    std::cout << "elapsed time: " << timer.elapsed<micro::microseconds>() << std::endl;
    timer.restart();
    // do something here
    std::cout << "elapsed time: " << timer.elapsed<micro::milliseconds>() << std::endl;
    \endcode
  */
  class stopwatch final {
  private:

    clock_t begin_, end_;

  public:

    /** Creates stopwatch. */
    stopwatch():begin_(now()),end_(begin_) {}

    ~stopwatch() {}

    /** Restarts stopwatch. */
    inline void restart() { reset(now()); }

    /** Stops stopwatch. */
    inline void stop() { end_ = now(); }

    /** Resets stopwatch. */
    inline void reset(clock_t x) { begin_ = end_ = x; }

    /** \returns Reference to begin of measure. */
    inline clock_t& begin() { return begin_; }

    /** \returns Reference to end of measure. */
    inline clock_t& end() { return end_; }

    /** \returns Elapsed time with precision T for this stopwatch. \param[in] do_stop stop timer or do not */
    template<typename T = milliseconds>
    inline int elapsed(bool do_stop = false) {
      clock_t end = now();
      return duration<T>(begin_, do_stop ? (end_ = end) : end);
    }

    /** \returns Result time with precision T for this stopwatch. */
    template<typename T = milliseconds>
    inline int result() { return duration<T>(begin_, end_); }

  };

} // namespace micro

#endif // time_hpp_included
