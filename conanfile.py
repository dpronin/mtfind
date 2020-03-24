from conans import ConanFile, CMake, tools

class Mtfind(ConanFile):
    name = "mtfind"
    version = "0.1"
    description = "A tool for finding substrings in a file"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    requires = "boost/[~1.65]@conan/stable"

    build_requires = \
        "gtest/[~1.8]@bincrafters/stable", \
        "benchmark/[~1.5]"
