# shift
This is the official repository for the 'shift' programming language. This project is still under heavy development with many missing or incomplete features; experimental behaviour is to be expected.

# Introduction

Welcome to the documentation for our custom programming language! In this document, you will find information on how to get started with the language, as well as an overview of its features and syntax.

# Building
This section will give further directions on how to build the programming language on numerous platforms.

# Getting Started

To use our custom programming language, you will need to install the language interpreter on your machine. The interpreter is available for Windows, macOS, and Linux.

Once you have installed the interpreter, you can start writing programs in our custom language. To run a program, simply open a terminal, navigate to the directory where your program is located, and enter the command `<interpreter> <program_name>`, where `<interpreter>` is the path to the interpreter executable and `<program_name>` is the name of your program file.

# Language Features

Our custom programming language is an object-oriented language with a strong emphasis on simplicity and readability. Some of its notable features include:

- Support for variables and data types such as integers, floats, and strings
- Support for control structures such as `if` statements and `for` loops
- Support for object-oriented programming, including classes, inheritance, and polymorphism
- A standard library of functions for common tasks such as input/output, mathematics, and string manipulation

# Syntax Overview

Here is a brief overview of the syntax of our custom programming language:

## Variables

Variables are declared using the `var` keyword, followed by the variable name and an optional initialization value. For example:

```cpp
var x = 10
var y = "Hello, World!"
```

## Control Structures

Control structures such as `if` statements and `for` loops are written using familiar syntax:
```cpp
if x > 10:
print("x is greater than 10")
else:
print("x is less than or equal to 10")

for i in range(5):
print(i)
```

## Functions

Functions are defined using the `def` keyword, followed by the function name and a list of parameters in parentheses. The body of the function is indented. For example:

```cpp
def greet(name):
print("Hello, " + name)

greet("Alice")
```

## Classes

Classes are defined using the `class` keyword, followed by the class name and an optional list of base classes. The body of the class is indented. For example:

```cpp
class Person:
def init(self, name):
self.name = name

def greet(self):
    print("Hello, my name is " + self.name)

p = Person("Alice")
p.greet()
```

# Conclusion

We hope this documentation has given you a good overview of our custom programming language and how to get started with it. For more information, please refer to the language reference manual or the examples in the standard library.

