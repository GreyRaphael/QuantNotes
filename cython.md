# cython

- [cython](#cython)
  - [wrapper pure python](#wrapper-pure-python)
  - [cython inherit c++ class](#cython-inherit-c-class)
  - [inherit c++ pure virtual class](#inherit-c-pure-virtual-class)
    - [simple example](#simple-example)
    - [only build library](#only-build-library)
    - [pybind11 build whl](#pybind11-build-whl)

## wrapper pure python

```bash
.
├── pkg01
│   ├── aaa01.py
│   └── aaa02.py
├── pkg02
│   ├── bbb01.py
│   └── bbb02.py
├── setup.py
└── xxx.py
```

```py
# aaa01.py
def funcaa1(x):
    return x + x
```

```py
# aaa02.py
def funcaa2(x, y):
    return x - y
```

```py
# bbb01.py
from pkg01 import aaa01

def funcbb1(x):
    return x * aaa01.funcaa1(x)
```

```py
# bbb02.py
def funcbb2(x, y):
    return x / y
```

```py
# xxx.py
from pkg02 import bbb01

def funcxx(x):
    return bbb01.funcbb1(x)
```

`python setup.py bdist_wheel` or `python setup.py build`

```py
from setuptools import setup
from Cython.Build import cythonize

setup(
    name="xxx",
    version="1.0.0",
    ext_modules=cythonize("xxx.py", compiler_directives={"language_level": "3"}),
)
```

## cython inherit c++ class

```bash
.
├── BaseClass.h
├── DerivedClass.h
├── NextDerivedClass.h
├── proj1.pyx
├── setup.py
└── test.py
```

```cpp
// BaseClass.h
#pragma once

#include <cstdio>
#include <iostream>
#include <string>

class BaseClass {
   public:
    BaseClass() {};
    virtual ~BaseClass() {};
    virtual void SetName(std::string name) {
        printf("BASE: in SetName, name=%s\n", name.c_str());
    }
    virtual float Evaluate(float time) {
        printf("BASE: in Evaluate\n");
        return time * 10;
    }
    virtual bool DataExists() {
        printf("BASE: in data exists\n");
        return false;
    }
};
```

```cpp
// DerivedClass.h
#pragma once

#include <string>
#include "BaseClass.h"

class DerivedClass : public BaseClass {
   public:
    DerivedClass() {};
    virtual ~DerivedClass() {};
    virtual void SetName(std::string name) override {
        printf("DERIVED CLASS: in SetName, name=%s\n", name.c_str());
    }
    virtual float Evaluate(float time) override {
        printf("DERIVED CLASS: in Evaluate\n");
        return time * 100;
    }
    virtual bool DataExists() override {
        printf("DERIVED CLASS:in data exists\n");
        return true;
    }
    virtual void MyFunction() { printf("DERIVED CLASS: in my function\n"); }
};
```

```cpp
// NextDerivedClass.h
#include "DerivedClass.h"

class NextDerivedClass : public DerivedClass {
   public:
    NextDerivedClass() {};
    virtual ~NextDerivedClass() {};
    virtual float Evaluate(float time) override {
        printf("NEXT DERIVED CLASS: in Evaluate\n");
        return time * 1000;
    }
};
```

```py
# proj1.pyx
#Necessary Compilation Options
#distutils: language = c++
#distutils: extra_compile_args = ["-std=c++11", "-g"]

#Import necessary modules
from libcpp cimport bool
from libcpp.string cimport string

cdef extern from "BaseClass.h":
    cdef cppclass BaseClass:
        BaseClass() except +
        void SetName(string)
        float Evaluate(float)
        bool DataExists()

cdef extern from "DerivedClass.h":
    cdef cppclass DerivedClass(BaseClass):
        DerivedClass() except +
        void MyFunction()

cdef extern from "NextDerivedClass.h":
    cdef cppclass NextDerivedClass(DerivedClass):
        NextDerivedClass() except +

cdef class PyBaseClass:
    cdef BaseClass *thisptr
    def __cinit__(self):
        #if type(self) is PyBaseClass:
        self.thisptr = new BaseClass()
    def __dealloc__(self):
        if type(self) is PyBaseClass:
            del self.thisptr
    def SetName(self, name):
        self.thisptr.SetName(name)
    def Evaluate(self, time):
        return self.thisptr.Evaluate(time)
    def DataExists(self):
        return self.thisptr.DataExists()

cdef class PyDerivedClass(PyBaseClass):
    cdef DerivedClass *derivedptr
    def __cinit__(self):
        # if type(self) is PyDerivedClass:
        self.derivedptr = self.thisptr = new DerivedClass()
    def __dealloc__(self):
        if type(self) is PyBaseClass:
            del self.derivedptr
    def SetName(self, name):
        self.derivedptr.SetName(name)
    def MyFunction(self):
        self.derivedptr.MyFunction()

cdef class PyNextDerivedClass(PyDerivedClass):
    cdef NextDerivedClass *nextDerivedptr
    def __cinit__(self):
        self.nextDerivedptr = self.derivedptr = self.thisptr = new NextDerivedClass()
    def __dealloc__(self):
        del self.nextDerivedptr
```

`python setup.py build_ext --inplace`

```py
# setup.py
from setuptools import setup
from Cython.Build import cythonize

setup(ext_modules=cythonize("proj1.pyx"))
```

```py
# test.py to use
from proj1 import PyBaseClass, PyDerivedClass, PyNextDerivedClass


class MyClass(PyBaseClass):
    def DataExists(self):
        print("in MyClass")
        return False


class MyClass2(PyDerivedClass):
    def DataExists(self):
        print("in MyClass2")
        return True


if __name__ == "__main__":
    obj1 = PyBaseClass()
    obj1.SetName(b"Bob")
    print(obj1.Evaluate(10))
    print(obj1.DataExists())

    print("-" * 60)
    obj2 = PyDerivedClass()
    obj2.SetName(b"Tom")
    print(obj2.Evaluate(10))
    print(obj2.DataExists())
    obj2.MyFunction()

    print("-" * 60)
    obj3 = PyNextDerivedClass()
    obj3.SetName(b"Moris")
    print(obj3.Evaluate(10))
    print(obj3.DataExists())
    obj3.MyFunction()

    print("-" * 60)
    obj4 = MyClass()
    obj4.SetName(b"xiaohong")
    print(obj4.Evaluate(10))
    print(obj4.DataExists())

    print("-" * 60)
    obj5 = MyClass2()
    obj5.SetName(b"James")
    print(obj5.Evaluate(10))
    print(obj5.DataExists())
    obj5.MyFunction()
```

## inherit c++ pure virtual class

> recommended for pybind11, not recommended for cython, [notes](https://zyxin.xyz/blog/2019-08/glue-python-cpp/#boostpythonpybind11-vs-cython)

### simple example

`pip install pybind11`

```bash
main.cpp
setup.py
```

```py
# setup.py
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import pybind11

ext_modules = [
    Extension(
        name="proj1",
        sources=[
            "main.cpp",
        ],
        include_dirs=[
            pybind11.get_include(),  # Path to pybind11 headers
        ],
        language="c++",
        extra_compile_args=["-std=c++20"],  # depends on compiler
    ),
]

setup(
    name="proj1",
    version="1.0.0",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
```

```cpp
// main.cpp
#include <pybind11/pybind11.h>

PYBIND11_MODULE(proj1, m) {
    m.def("subi", [](int i, int j) { return i - j; }, "integer substract y from x, proj1.sub(x,y)");
    m.def("subd", [](double i, double j) { return i - j; }, "double substract y from x, proj1.sub(x,y)");
}
```

### only build library

simple example with override c++ virtual functions

```bash
.
├── main.cpp
└── test.py
```

```cpp
// main.cpp
#include <pybind11/pybind11.h>
#include <memory>

class Base {
   public:
    virtual ~Base() = default;
    virtual int work(int x) = 0;

    int start() {
        int sum = 0;
        for (int i = 0; i < 4; ++i) {
            sum += work(i);
        }
        return sum;
    }
};

class PyBase : public Base {
   public:
    using Base::Base;  // Inherit constructors

    int work(int x) override {
        PYBIND11_OVERRIDE_PURE(
            int,  /* Return type */
            Base, /* Parent class */
            work, /* Name of function in C++ (must match Python name) */
            x     /* Argument(s) */
        );
    }
};

PYBIND11_MODULE(strategy, m) {
    pybind11::class_<Base, PyBase, std::shared_ptr<Base>>(m, "BaseStrategy")
        .def(pybind11::init<>())
        .def("start", &Base::start)
        .def("work", &Base::work);
}
```

```bash
# build library
# g++ -O3 -Wall -shared -std=c++20 -fPIC `python3 -m pybind11 --includes` main.cpp -o strategy`python3-config --extension-suffix`
# c++ -O3 -Wall -shared -std=c++20 -fPIC `python3 -m pybind11 --includes` main.cpp -o strategy`python3-config --extension-suffix`
clang++ -O3 -Wall -shared -std=c++20 -fPIC `python3 -m pybind11 --includes` main.cpp -o strategy`python3-config --extension-suffix`
```

```py
# test.py
from strategy import BaseStrategy

class Strategy1(BaseStrategy):
    def work(self, x):
        return x * x

# error
class Strategy2(BaseStrategy):
    pass

class Strategy3(BaseStrategy):
    def work(self, x):
        return x * x * x

obj1 = Strategy1()
print(obj1.start())  # 14

# obj2 = Strategy2()
# print(obj2.start())  # error, unimplemented

obj3 = Strategy3()
print(obj3.start())  # 36
```

### pybind11 build whl

```bash
.
├── main.cpp # same as above
├── setup.py
└── test.py # sample as above
```

```py
# setup.py
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import pybind11

ext_modules = [
    Extension(
        name="strategy",
        sources=[
            "main.cpp",
        ],
        include_dirs=[
            pybind11.get_include(),  # Path to pybind11 headers
        ],
        language="c++",
        extra_compile_args=["-std=c++20"],  # depends on compiler
        # py_limited_api=True, # not support in pybind11, but in cython
        # define_macros=[("Py_LIMITED_API", "0x03090000")], # not support in pybind11, but in cython
    ),
]

setup(
    name="strategy",
    version="1.0.0",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
```

- `python setup.py bdist_wheel` or `python setup.py build`
- `auditwheel show dist/*.whl`: check manylinux platform
- `auditwheel repair dist/*.whl --plat manylinux_2_24_x86_64`

```bash
.
├── build
│   ├── bdist.linux-x86_64
│   ├── lib.linux-x86_64-3.9
│   │   └── strategy.cpython-39-x86_64-linux-gnu.so
│   └── temp.linux-x86_64-3.9
│       └── main.o
├── dist
│   └── strategy-1.0.0-cp39-cp39-linux_x86_64.whl
├── main.cpp
├── setup.py
├── strategy.egg-info
│   ├── dependency_links.txt
│   ├── not-zip-safe
│   ├── PKG-INFO
│   ├── SOURCES.txt
│   └── top_level.txt
├── test.py
└── wheelhouse
    └── strategy-1.0.0-cp39-cp39-manylinux_2_34_x86_64.whl # target file
```

```bash
pip install wheelhouse/*.whl
python test.py
pip uninstall strategy
```