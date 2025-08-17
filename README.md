# Ferrum

## Build
- git clone https://github.com/yayl/ferrum
- cd ferrum/c-compiler
- ./start.sh c

## Usage
- ./start.sh \[module\]

## Optional Requirements
We use libbacktrace for backtraces but it can be disabled by commenting out *BACKTRACE* in common/defines.h and also removing it from CMakeLists.txt


## The definition of different terms used in ferrum:

### Module: 
    The AST representation of a file of ferrum code 
### Symbol: 
    ID that is used to refer to a definition in the module scope
### Type:
    A symbol that refers to an ordering of data
### Marker:
    A symbol that defines a type
### Trait:
    A symbol that defines a property of type
### Struct:
    A marker that is named structuring of markers
### Enum:
    A marker capable of being different types at runtime
### Impl:
    An implementation of a trait on a type
