/** \file iinfo.hpp */
#ifndef IINFO_HPP_INCLUDED
#define IINFO_HPP_INCLUDED

namespace micro {

  /**
    \class iinfo
    \brief Interface for plugin type
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    Plugin's type interface.
  */
  class iinfo {
  private:

    const std::type_info& info_;

  public:

    iinfo(const std::type_info& i):info_(i) {}

    virtual ~iinfo() {}

    const std::type_info& type_info() const noexcept { return info_; }

  };

} // namespace micro

#endif // IINFO_HPP_INCLUDED
