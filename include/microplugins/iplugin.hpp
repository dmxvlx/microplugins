/** \file iplugin.hpp */
#ifndef IPLUGIN_HPP_INCLUDED
#define IPLUGIN_HPP_INCLUDED

#include "storage.hpp"

namespace micro {

  template<std::size_t> class iplugins;

  /**
    \class iplugin
    \brief Interface for plugins
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    Plugin's interface.

    \example plugin1.cxx
  */
  template<std::size_t L = MAX_PLUGINS_ARGS>
  class iplugin : public storage<L> {
  private:

    template<std::size_t> friend class plugins;

    std::atomic<bool> do_work_;
    std::shared_ptr<iplugins<L>> plugins_;

  protected:

    explicit iplugin(int v, const std::string& nm):storage<L>(v, nm),do_work_(false),plugins_(nullptr) {}

  public:

    ~iplugin() override {}

    /** \returns Shared pointer to this plugin. \see std::enable_shared_from_this */
    virtual std::shared_ptr<iplugin<L>> get_shared_ptr() { return nullptr; }

    /** \returns State of service of this plugin. \retval true if service can continue do work \retval false if service of this plugin must be interrupted/stopped */
    inline bool is_run() const { return do_work_; }

    /** \returns Shared pointer to plugins kernel \see iplugins */
    inline std::shared_ptr<iplugins<L>> get_plugins() { return plugins_; }

  };

} // namespace micro

/** Signature of "C" function for loading plugin from dll. \see import_plugin() */
using import_plugin_cb_t = std::shared_ptr<micro::iplugin<>>();

extern "C" {
  /** \returns Shared pointer to newly created instance of a plugin from dll. */
  extern std::shared_ptr<micro::iplugin<>> import_plugin();
}

#endif // IPLUGIN_HPP_INCLUDED
