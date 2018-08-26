/** \file singleton.hpp */
#ifndef singleton_hpp_included
#define singleton_hpp_included

#include <memory>
#include <mutex>

namespace micro {

  /**
    \class singleton
    \brief Realization template version of the singleton pattern
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    Thread safe singleton, which returns shared pointer to the instance of an object.

    \code c++
    #include <microplugins/singleton.hpp>
    #include <string>

    class myclass final : public micro::singleton<myclass> {
    private:

      // must be friend (ctor is private)
      fiend class micro::singleton<myclass>;

      // put the ctor into private section
      myclass(const std::string& param1):micro::singleton<myclass>() {}

    public:

      // dtor in the singleton was declared as virtual - marks it by override
      ~myclass() override {}

      void method1() {}

    };


    // and using:
    int main() {
      std::shared_ptr<myclass> ptr = myclass::get("param1"); // create the instance

      // ...
      // now we can get the instance by myclass::get() method in any place of app
      // ...

      return 0;
    }
    \endcode
  */
  template<typename T>
  class singleton {
  private:

    static std::shared_ptr<T> instance_;

    singleton(const singleton<T>& rhs) = delete;

    singleton(singleton<T>&& rhs) = delete;

    singleton<T>& operator=(const singleton<T>& rhs) = delete;

    singleton<T>& operator=(singleton<T>&& rhs) = delete;

  public:

    /** \returns Shared pointer to instance of T. \param[in] args arguments depends on Constructor of T */
    template<typename... Ts>
    static std::shared_ptr<T> get(Ts&&... args) {
      static std::recursive_mutex mtx = {};
      if (!instance_.get()) {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        if (!instance_.get()) instance_.reset(new T(args...));
      } return instance_;
    }

  protected:

    singleton() {}

    virtual ~singleton() {}

  };

  template<typename T> std::shared_ptr<T> singleton<T>::instance_ = nullptr;

} // namespace micro

#endif // singleton_hpp_included
