# Ferrum

## How to setup:
- git clone https://github.com/YayL/Ferrum
- cd Ferrum/c-compiler
- mkdir build
- git submodule init
- git submodule update
- ./start.sh c

After following the steps above you have a working version of Ferrum to use at ./build/compiler



## The definition of different terms used in ferrum:

### Module: The AST representation of a file of ferrum code 

### Symbol: ID that is used to refer to a definition in the module scope

### Type: A symbol that refers to an ordering of data

### Marker: A symbol that defines a type

### Trait: A marker that is a group of types

### Struct: A marker that is named structuring of markers

### Enum: A marker capable of being different types at runtime

### Impl: A developer implemented code snippet defining a declared type's "member functions"
