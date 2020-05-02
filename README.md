# Compiler of Alan

### Description of Alan language:
The basic characteristics of Alan language are the following:
* Simple structure and syntax 
* Basic types of data for integers (int, byte) and one-dimensional array
* Simple functions, arguments by value or by reference
* Pascal-like variable scope
* Library of basic functions

### Requirements
Install the following required libraries:
```sh
$ sudo apt-get install flex
```
```sh
$ sudo apt-get install bison
```
```sh
$ sudo apt-get install clang-6.0 llvm-6.0 llvm-6.0-dev llvm-6.0-tools
```

### Build

To build the compiler run:
```sh
$ make
```
An executable called 'alanc' is generated

### Run

To compile a file 'example.alan':
```sh
$ ./compile.sh example.alan
```

An executable called 'example.out' is generated

Run the executable:
```sh
$ ./example.out
```
