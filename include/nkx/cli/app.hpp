#pragma once

#include <CLI/CLI.hpp>
#include <memory>
#include <string>

namespace nkx {

class App {
public:
    App();
    
    // Runs the CLI with given arguments. Returns exit code.
    int run(int argc, char** argv);

private:
    void setup_scan();
    void setup_inspect();
    void setup_convert();
    void setup_install();
    void setup_doctor();

    std::unique_ptr<CLI::App> _app;
};

} // namespace nkx
