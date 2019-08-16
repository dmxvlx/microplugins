#ifndef IINFO_HPP_INCLUDED
#define IINFO_HPP_INCLUDED

namespace micro {

  class iinfo {
  private:

    const std::type_info& info_;

  public:

    iinfo(const std::type_info& i):info_(i) {}

    virtual ~iinfo() {}

    const std::type_info& type_info() { return info_; }

  };

} // namespace micro

#endif // IINFO_HPP_INCLUDED
