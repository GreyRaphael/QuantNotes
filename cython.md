# cython

- [cython](#cython)
  - [wrapper pure python](#wrapper-pure-python)
  - [cython inherit c++ class](#cython-inherit-c-class)

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