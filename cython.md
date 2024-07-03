# cython

- [cython](#cython)
  - [wrapper pure python](#wrapper-pure-python)

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