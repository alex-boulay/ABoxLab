#include "ABoxLabApp.hpp"
#include "utils/Logger.hpp"
#include <exception>

int main() {
  ABoxLabApp app;
  LOG_INFO("App") << "App created, memory pointer: " << (void *)&app;
  try {
    app.run();
  } catch (const std::exception &e) {
    LOG_ERROR("App") << e.what();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
