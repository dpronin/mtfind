# Cloning repository
```bash
bash> git clone git@github.com:dannftk/mtfind.git
```
# Build requirements
- [cmake](https://cmake.org/) to configure the project. Minimum required version is __3.14__
- [conan](https://conan.io/) to download all the dependencies of the application and configure with a certain set of parameters. You can install __conan__ by giving a command to __pip__:
    ```bash
        bash> pip install --user conan
    ```
    To use __pip__ you need to install __python__ interpreter. I highly recommend to install __python3__-based versions in order to avoid unexpected results with __conan__

- A C++ compiler with __C++17__, __boost-1.76__, __gtest-1.11__, __google-benchmark-1.5__ support

The package has been successfully tested on compilation with __gcc-10.3__ (g++ with libstdc++11), __llvm-clang-12.0__ (clang++ with libc++)

# Preparing with conan
First you need to set __conan's remote list__ to be able to download packages prescribed in the `conanfile.py` as requirements (dependencies). You need at least one remote known by conan. We need __conan-center__ repository available. To check if it already exists run the following command:
```bash
bash> conan remote list
```
If required remote is already there you will see output alike:
```bash
bash> conan remote list
conan-center: https://conan.bintray.com [Verify SSL: True]
```
If one doesn't appear you should set it by running the command:
```bash
bash> conan remote add conan-center https://conan.bintray.com
```
Since now you're ready to perform conan installation.

    WARNING: if you have variables CC or/and CXX set in your environment you need to unset them before executing next commands, otherwise, if conan decides to build a dependency on host the compiler selected from parameters and compiler from CC/CXX may contradict, as the result some cmake configuring processes while bulding dependencies may fail

If you have resolved all the possible issues described above, you can start installation with conan. Below there is installed a conan's environment with making use of __gcc-10.3__, __libstdc++11__, architecture __x86\_64__. If something does not correspond to your environment (for example, __gcc__ is a lower version), change it. Installation with __gcc-10.3__, __libstdc++11__, __x86\_64__, __Debug mode__:
```
bash> cd dpiquic
bash> conan install . -if debug/ -s arch=x86_64 -s arch_build=x86_64 -s compiler=gcc -s compiler.version=10.3 -s compiler.libcxx=libstdc++11 -s build_type=Debug --build missing
```
All the parameters provided to conan are vital. By using them conan determines whether it can download already built binary package from a remote or it must build a dependency up on the host by itself (if `--build missing` parameter is passed)
By the time the command has finished you will have got `debug` directory in the root of the project

To install prerequisites for __Release mode__ leaving other parameters untouched, use the following command:
```bash
bash> conan install . -if release/ -s arch=x86_64 -s arch_build=x86_64 -s compiler=gcc -s compiler.version=10.3 -s compiler.libcxx=libstdc++11 -s build_type=Release --build missing
```
As just the command has worked out `release` directory will appear in the root of the project

You can vary conan's parameters according to your needs. For instance, if you like to build a package with __gcc-6.5__ provide the `-s compiler.version=6.5`leaving all the rest parameters untouched

To learn more about conan available actions and parameters consult `conan --help`

# Configuring and building with cmake

Suppose, you have prepared __Debug mode__ with conan and got `debug` directory.

    WARNING: Before configuring make sure you don't have CC/CXX environment variables set, otherwise they may contradict with those having been configured by conan

To configure the project with `cmake` run the commands:
```bash
bash> cd debug
bash> cmake -DCMAKE_BUILD_TYPE=Debug ../
```
The project is configured. To built it run:
```bash
bash> cmake --build .
```
To enable disable __unit and integrational tests__, provide an additional parameter `-DBUILD_TESTING=OFF` to __cmake__ while configuring

To enable building __benchmarks__, provide an additional parameter `-DENABLE_BENCH=ON` to __cmake__ while configuring

There are more parameters you can provide to cmake

If the compilation's finished successfully, in the directory `${project_root}/debug/bin/` you will find `mtfind` binary. In case tests have been enabled and built you will also find `mtfind_test` binary alongside

To configure project in __Release mode__ provide `-DCMAKE_BUILD_TYPE=Release` instead of `-DCMAKE_BUILD_TYPE=Debug`

__Release__ version accomplishes better performance results

Run the application with the example given in the __task.txt__ located in the root of the project:
```bash
bash> cd ${project_root}/debug/bin/
bash> ./mtfind ../../input.txt "?ad"
3
5 5 bad
6 6 mad
7 6 had
```

Gotcha!

If you have any questions do not hesitate to ask [me](mailto:dannftk@yandex.ru)
