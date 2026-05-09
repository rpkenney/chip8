#include "glfw_frontend.h"

#include <cstdio>
#include <cstring>

static void printUsage(const char* argv0) {
    std::fprintf(stderr,
                 "usage: %s [--trace] [--step] [-b|--breakpoints FILE] ROM\n",
                 argv0);
}

int main(int argc, char* argv[]) {
    glfw_frontend::Options opts;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--trace") == 0) {
            opts.trace = true;
        } else if (std::strcmp(argv[i], "--step") == 0) {
            opts.step = true;
        } else if (std::strcmp(argv[i], "--breakpoints") == 0 ||
                   std::strcmp(argv[i], "-b") == 0) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s: %s requires an argument\n", argv[0], argv[i]);
                printUsage(argv[0]);
                return 1;
            }
            opts.breakpoints_path = argv[++i];
        } else if (argv[i][0] == '-') {
            std::fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
            printUsage(argv[0]);
            return 1;
        } else if (opts.rom_path == nullptr) {
            opts.rom_path = argv[i];
        } else {
            std::fprintf(stderr, "%s: unexpected argument %s\n", argv[0], argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }
    if (opts.rom_path == nullptr) {
        std::fprintf(stderr, "%s: missing ROM path\n", argv[0]);
        printUsage(argv[0]);
        return 1;
    }

    return glfw_frontend::run(opts);
}
