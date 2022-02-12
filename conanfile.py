# how to use semver https://devhints.io/semver

from conans import ConanFile, CMake, tools

class Mtfind(ConanFile):
    name = "mtfind"
    version = "1.1.0"
    author = "Denis Pronin"
    url = "https://github.com/dpronin/mtfind"
    description = "A tool for finding substrings in a file"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"

    scm = {
        "type": "git",
        "subfolder": name,
        "url": "auto",
        "revision": "auto",
        "username": "git"
    }

    def requirements(self):
        self.requires("boost/[~1.78]")

    def build_requirements(self):
        self.build_requires("gtest/[~1.11]")
        self.build_requires("benchmark/[~1.6]")

    def _configure(self, enable_test = False, enable_bench = False, verbose = True):
        cmake = CMake(self)
        cmake.verbose = verbose
        cmake.definitions['CMAKE_BUILD_TYPE'] = "Debug" if self.settings.build_type == "Debug" else "Release"
        cmake.definitions['ENABLE_BENCH'] = enable_bench
        cmake.definitions['ENABLE_GDB_SYMBOLS'] = self.settings.build_type == "Debug"
        cmake.configure(source_folder = self.name)
        return cmake

    def build(self):
        cmake = self._configure(enable_test = True)
        cmake.build()
        cmake.test(output_on_failure = True)

    def package(self):
        cmake = self._configure()
        cmake.install()
