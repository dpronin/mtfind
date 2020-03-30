# Cloning repository
```bash
bash> git clone git@github.com:dannftk/mtfind.git
```
# Build requirements
- [conan](https://conan.io/) to download all the dependencies of the application and configure with a certain set of parameters. You can install __conan__ by giving a command to __pip__:
    ```bash
        bash> pip install --user conan
    ```
    To use __pip__ you need to install __python__ interpreter. I highly recommend to install __python3__-based versions in order to avoid unexpected results with __conan__ 

- A C++ compiler with __C++14__ and __boost-1.65.1__ support. The package has been successfully tested on compilation with __gcc-9.2__ (g++ with libstdc++11)

# Preparing with conan
First you need to set __conan's remote list__ to be able to download packages prescribed in the `conanfile.py` as requirements (dependencies). You need at least two remotes known by conan. We need __conan-center__ and __bincrafters__ repositories available. To check them if they already exist run the following command:
```bash
bash> conan remote list
```
If required remotes are already there you will see output alike:
```bash
bash> conan remote list
bincrafters: https://api.bintray.com/conan/bincrafters/public-conan [Verify SSL: True]
conan-center: https://conan.bintray.com [Verify SSL: True]
```
If one or both of them don't appear you should set them by running the commands:
```bash
bash> conan remote add conan-center https://conan.bintray.com
bash> conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
```
Since now you're ready to perform conan installation. 
    
    WARNING: if you have variables CC or/and CXX set in your environment you need to unset them before executing next commands, otherwise, if conan decides to build a dependency on host the compiler selected from parameters and compiler from CC/CXX may contradict, as the result some cmake configuring processes while bulding dependencies may fail

If you have resolved all the possible issues described above, you can start installation with conan. Below there is installed a conan's environment with making use of __gcc-9.2__, __libstdc++11__, architecture __x86\_64__. If something does not correspond to your environment (for example, __gcc__ is a lower version), change it. Installation with __gcc-9.2__, __libstdc++11__, __x86\_64__, __Debug mode__:
```
bash> cd mtfind
bash> conan install . -if debug/ -s arch=x86_64 -s arch_build=x86_64 -s compiler=gcc -s compiler.version=9.2 -s compiler.libcxx=libstdc++11 -s build_type=Debug --build missing
```
All the parameters provided to conan are vital. By using them conan determines whether it can download already built binary package from a remote or it must build a dependency up on the host by itself (if `--build missing` parameter is passed)
After the command has finished you will get `debug` directory in the root of the project 

To install prerequisites for __Release mode__ leaving other parameters untouched, use the following command:
```bash
bash> conan install . -if release/ -s arch=x86_64 -s arch_build=x86_64 -s compiler=gcc -s compiler.version=9.2 -s compiler.libcxx=libstdc++11 -s build_type=Release --build missing
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
To enable building __unit tests__, provide an additional parameter `-DENABLE_TEST=ON` to __cmake__ while configuring

If the compilation's finished successfully, in the directory `${project_root}/debug/bin/` you will find `mtfind` binary. In case tests have been enabled and built you will also find `mtfind_test` binary alongside

To configure project in __Release mode__ provide `-DCMAKE_BUILD_TYPE=Release` instead of `-DCMAKE_BUILD_TYPE=Debug`

Release version accomplishes better performance results

Run the application with the example given in the task.txt:
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
