#ifndef plugin1_cxx
#define plugin1_cxx

#include "iplugins.hpp"
#include <iostream>

// note: in microplugins, all returning values
// and arguments of tasks are std::any type;
// after running a task it returns std::shared_future<std::any> type
// because the tasks will launched in async mode, see std::async


[[maybe_unused]] static std::any service(std::any a1) {
  // for the service task in plugins, first argument is pointer to the plugin
  std::shared_ptr<micro::iplugin<>> self = std::any_cast<std::shared_ptr<micro::iplugin<>>>(a1);
  std::shared_ptr<micro::iplugins<>> manager = self->get_plugins();

  std::clog << "kernel version: " << manager->major() << "." << manager->minor() << std::endl;
  std::clog << "kernel name: " << manager->name() << std::endl;

  // do work while plugin's service in actived mode (managed by manager)
  while (self->is_run()) {
    std::shared_ptr<micro::iplugin<>> other_plugin = manager->get_plugin("other_plugin");

    if (other_plugin) {
      // do things with newly loaded plugin from the manager (or do any calculations)
      if (other_plugin->has<0>("help")) {
        std::shared_future<std::any> result = other_plugin->run<0>("help");
        // we can pass the result as argument into any function ...
        result.wait(); // needs wait for calculation the result
        if (result.valid() && result.get().has_value() && result.get().type() == typeid(std::string)) {
          std::clog << std::any_cast<std::string>(result.get()) << std::endl;
        }
      }
    }

    // or we can call a task with two arguments from manager
    if (manager->has<2>("some_task_name")) { /* ... */ }

    // avoid loading CPU, needs to sleep least for 1 millisecond
    micro::sleep<micro::seconds>(1);
  }

  return {}; // result from a service will not processed anyway
}


// simple help message for test0 task
static const char test0_help[] = {
  "author: Dmitrij Volin\n"
  "function: test0()\n"
  "description: simple test function for plugin1\n\n"
  "arguments: none\n"
  "returns: std::string covered by std::shared_future<std::any>"
};


// simple test0 task
static std::any test0() {
  std::clog << "test0" << std::endl;
  return std::make_any<std::string>("hello from test0");
}


// another task with few arguments
static std::any sum2(std::any a1, std::any a2) {
  if (!a1.has_value() || !a2.has_value()) return 0; // error
  else if (a1.type() != typeid(int) || a2.type() != typeid(int)) return 0; // error
  else return std::any_cast<int>(a1) + std::any_cast<int>(a2); // all is ok
}


// our class plugin
class plugin1 final : public micro::iplugin<>, public std::enable_shared_from_this<plugin1> {
public:

  plugin1(int v, const std::string& nm):micro::iplugin<>(v, nm),
  std::enable_shared_from_this<plugin1>() {

    // warning: in this moment, plugin has no manager in micro::iplugin::plugins_ !

    // register functions for our plugin in the ctor
    subscribe<0>("test0", test0, test0_help);
    subscribe<2>("sum2", sum2);

    // next line call will not have an effect,
    // task with name `sum2' exists in the plugin already, see unsubscribe
    subscribe<2>("sum2", sum2);

    subscribe<1>("method1", std::bind(&plugin1::method1, this, std::placeholders::_1));
    subscribe<0>("lambda0", []()->std::any{return std::make_any<std::string>("hello from lambda0 !");});

    // note: we can put any type of objects for arguments our tasks,
    // with std::make_any<MyClass>(args for MyClass's ctor)

    // if a plugin has service, then it will called once after loading
    // if we want to have our plugin as service - we need to uncomment next line
    // subscribe<1>("service", service);
  }

  ~plugin1() override {}

  std::shared_ptr<micro::iplugin<>> get_shared_ptr() override {
    return std::shared_ptr<micro::iplugin<>>(shared_from_this());
  }

  std::any method1(std::any a1) {
    std::clog << std::any_cast<std::string>(a1) <<std::endl;
    return std::make_any<std::string>("hello from method1 !");
  }

};


// instance of the our plugin
static std::shared_ptr<plugin1> instance = nullptr;


// extern function, that declared in "iplugin.hpp", for export the plugin from dll
std::shared_ptr<micro::iplugin<>> import_plugin() {
  return instance ? instance : (instance = std::make_shared<plugin1>(micro::make_version(1,0), "plugin1"));
}

#endif // plugin1_cxx
