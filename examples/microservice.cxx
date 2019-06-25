#ifndef microservice_cxx
#define microservice_cxx

#include "plugins.hpp"
#include <csignal>


static std::any service(std::any a1) {
  int ret = 0;
  std::shared_ptr<micro::plugins> manager = std::any_cast<std::shared_ptr<micro::plugins>>(a1);
  // we can do loop while manager->is_run() - for real service ...
  if (manager->is_run()) {
    std::shared_ptr<micro::iplugin> plugin1 = manager->get_plugin("plugin1");
    if (plugin1) {
      std::clog << "plugin1 is loaded ..." << std::endl;

      std::shared_future<std::any> r1, r2, r3, r4;

      r1 = plugin1->run<0>("test0");
      r2 = plugin1->run<2>("sum2", 25, 25);
      r3 = plugin1->run<1>("method1", std::make_any<std::string>("method1 running ..."));
      r4 = plugin1->run<0>("lambda0");

      r1.wait(); r2.wait(); r3.wait(), r4.wait();

      std::clog << "task `plugin1::test0()' returned: " << std::any_cast<std::string>(r1.get()) << std::endl;
      std::clog << "task `plugin1::sum2(25, 25)' returned: " << std::any_cast<int>(r2.get()) << std::endl;
      std::clog << "task `plugin1::method1(...)' returned: " << std::any_cast<std::string>(r3.get()) << std::endl;
      std::clog << "task `plugin1::lambda0()' returned: " << std::any_cast<std::string>(r4.get()) << std::endl;
    } else {
      std::clog << "can't load plugin1" << std::endl;
      ret = -1;
    }
    manager->stop();
  }
  return ret;
}


// signal handler
static void signal_handler(int s) {
  micro::plugins::get()->stop();
  std::exit(s);
}


int main(/*int argc, char* argv[]*/) {
  std::signal(SIGABRT, &signal_handler);
  std::signal(SIGTERM, &signal_handler);
  std::signal(SIGKILL, &signal_handler);
  std::signal(SIGQUIT, &signal_handler);
  std::signal(SIGINT, &signal_handler);

  std::shared_ptr<micro::plugins> plugins = micro::plugins::get(); // create instance
  plugins->subscribe<1>("service", service); // service task is optionally

  // set max idle to 3 minutes - if no one task will not called in that period,
  // for each loaded plugin - it will be unloaded (0 - unlimited, default - 10)
  plugins->max_idle(3);
  plugins->run(); // run thread for manage plugins
  while (plugins->is_run()) {
    micro::sleep<micro::milliseconds>(250);
  }

  return plugins->error();
}

#endif // microservice_cxx
