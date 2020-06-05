# how to use semver https://devhints.io/semver

from conans import ConanFile, CMake, tools

class Mtfind(ConanFile):
    name = "mtfind"
    version = "1.0.0"
    author = "Denis Pronin"
    url = "https://github.com/dannftk/mtfind"
    description = "A tool for finding substrings in a file"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    requires = "boost/[~1.73]"

    build_requires = \
        "gtest/[~1.10]", \
        "benchmark/[~1.5]"

    scm = {
        "type": "git",
        "subfolder": name,
        "url": "auto",
        "revision": "auto",
        "username": "git"
    }

    def _configure(self, enable_test = False, enable_bench = False, verbose = True):
        cmake = CMake(self)
        cmake.verbose = verbose
        cmake.definitions['CMAKE_BUILD_TYPE'] = "Debug" if self.settings.build_type == "Debug" else "Release"
        cmake.definitions['ENABLE_TEST'] = enable_test
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
