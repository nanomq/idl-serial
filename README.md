# idl-serial
Based IDL generate serial code:
- json to struct
- struct to json
## Overview
This project is a IDL serial generator for c. We use flex && bison to scan and parse files and generate json to struct and struct to json code. 
## Features
- [x] Primitive types
- [x] Structs
- [x] String and string with template
- [x] Sequence template 
- [x] Enums.
- [x] Part of [Annotations](https://cyclonedds.io/content/guides/supported-idl.html)
- [ ] Unions
- [ ] Enums
- [ ] Bitmasks
- [ ] Typedefs
- [ ] Modules
- [ ] Bitsets
- [ ] Constants

## Building
- Get the from github
```shell
git clone https://github.com/nanomq/idl-serial.git
```
- Build project
```shell
cd idl-serial && mkdir build
cd build && cmake ..
make
```

## Usage
- Generate code 
```shell
./idl-serial-code-gen ../test.idl
```
Code will generate to idl_convert.c.

