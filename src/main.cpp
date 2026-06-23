#include "nkx/cli/app.hpp"
#include <iostream>

int main(int argc, char** argv) {
    try {
        nkx::App app;
        return app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred\n";
        return 1;
    }
}
