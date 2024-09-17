from setuptools import setup, Extension
import pybind11

ext_modules = [
    Extension(
        "eldarcpp",
        ["src/bindings.cpp"],
        include_dirs=[pybind11.get_include()],
        language="c++",
        extra_compile_args=["-std=c++17", "-O3"],
    ),
]

setup(
    name="eldarcpp",
    version="0.0.0",
    author="Maixent Chenebaux",
    author_email="max.chbx@gmail.com",
    description="A blazing fast search engine with Python bindings written in C++",
    ext_modules=ext_modules,
    setup_requires=["pybind11>=2.5.0"],
    install_requires=["pybind11>=2.5.0"],
)
