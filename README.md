# Introduction
Microplugins is a C++ plugin framework for easy creating and using plugins.

* It supports services for plugins and communications between plugins kernel and other plugins.
* It uses a header-only design and makes it easy to integrate with existing projects.
* It takes care for unloading unused plugins automatically by given time.

# Requirements
* Compiler with support C++17 standart (including experimental filesystem)
* Cmake >= 2.6 (for build examples)
* Doxygen (for build documentation)

`This framework was tested on GNU/Linux with GCC-7.3.0`

# Installation
Compiling:
> $ mkdir build && cd build && cmake .. && make

Installation:
> $ make install

Or just copy folder *include/microplugins* into your project

# License
This library is distributed under the terms of the [Boost Software License - Version 1.0](LICENSE)

# Examples
```c++
#include <microplugins/plugins.hpp>


static std::any service(std::any a1) {
  std::shared_ptr<micro::plugins> k = std::any_cast<std::shared_ptr<micro::plugins>>(a1);

  std::shared_ptr<micro::iplugin> plugin1 = k->get_plugin("plugin1");

  if (plugin1 && plugin1->has<2>("sum2")) {
    std::shared_future<std::any> result = plugin1->run<2>("sum2", 125, 175);
    result.wait();
    std::cout << std::any_cast<int>(result.get()) << std::endl;
  }

  k->stop(); // stop if we don't need service mode
  return 0;
}


int main() {
  std::shared_ptr<micro::plugins> k = micro::plugins::get();
  k->subscribe<1>("service", service);

  k->run();

  while (k->is_run()) micro::sleep<micro::milliseconds>(250);

  return k->error();
}
```
You can see [example of plugin](examples/plugin1.cxx), and [example of service](examples/microservice.cxx)
