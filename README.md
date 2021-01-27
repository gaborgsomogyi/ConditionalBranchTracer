# Conditional Branch (Jump) Tracer for x64dbg

## Features
* Step over and step into trace possiblities
* Opens trace file and stores executed jump instructions in the following format
    ```
    7ff6ee9c702c: je 0x00007FF6EE9C703B not executed
    7ff6ee9c7039: jmp 0x00007FF6EE9C7025 executed
    ```
* Trace stopped only at breakpoints
* Trace file closed in the following conditions:
    * Stop tracing menu item clicked
    * Debugging session closed
* Writes trace files in chunks to minimize the time consuming file operations
