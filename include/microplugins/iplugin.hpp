/** \file iplugin.hpp */
#ifndef iplugin_hpp_included
#define iplugin_hpp_included

#include "storage.hpp"

namespace micro {

  class iplugins;

  /**
    \class iplugin
    \brief Interface for plugins
    \author Dmitrij Volin
    \date august of 2018 year
    \copyright Boost Software License - Version 1.0

    This interface needed for base of loading plugins.

    \example plugin1.cxx
  */
  class iplugin : public storage {
  private:

    friend class plugins;

    std::atomic<bool> do_work_;
    std::shared_ptr<iplugins> plugins_;

  protected:

    iplugin(float v, const std::string& nm):storage(v, nm),do_work_(false),plugins_(nullptr) {}

  public:

    ~iplugin() override {}

    /** \returns Shared pointer to this plugin. \see std::enable_shared_from_this */
    virtual std::shared_ptr<iplugin> get_shared_ptr() { return nullptr; }

    /** \returns State of service of this plugin. \retval true if service can continue do work \retval false if service of this plugin must be interrupted/stopped */
    bool is_run() const { return do_work_; }

    /** \returns Shared pointer to plugins kernel \see iplugins */
    std::shared_ptr<iplugins> get_plugins() { return plugins_; }

  };

} // namespace micro

/** Signature of "C" function for loading plugin from dll. \see import_plugin() */
using import_plugin_cb_t = std::shared_ptr<micro::iplugin>();

extern "C" {
  /** \returns Shared pointer to newly created instance of a plugin from dll. */
  extern std::shared_ptr<micro::iplugin> import_plugin();
}

#endif // iplugin_hpp_included
