# dll_vm

### We often choose between the right things and the easy ones。

## TODO

+ [x] test vm application name
+ [x] don't parse some data struct.
+ [x] test parsing
+ [x] write to new dex file
+ [x] test my dex file by apktool
+ [x] change AndroidManifest.xml
+ [x] design a new format of data file
+ [x] main shell step
+ [x] main vm code about setup system
+ [x] test repackaged App
+ [x] change java method to native method
+ [x] add vm interpreter
+ [x] test base vmp's App
+ [ ] randomized bytecode and update interpreter
+ [ ] relocate "relocate" and confusion
+ [x] add code cache(used map:{k: method id, v: CodeItem})
+ [x] hide stack
+ [x] stack in random address
+ [ ] cancel JNI call
+ [x] delete debug info in dex file
+ [x] memory cache
+ [ ] test App's profile

Additional:
+ [ ] register key functions dynamically and hide the map

## git log
    [2020-12-18]:   1. add class: Instruction
                    2. todo: mark the InstructionLinkType, and set goto's val.
                    3. todo: generating potential transformation equation according to bytecode frequency.
                    4. todo: generate instruction data (no bit exchange).
                    5. todo: analog instruction write, update offset value.
                    6. todo: update goto's value.
                    7. todo: instruction data bit transform and generate assignment statement.
                    8. todo: build the cpp and hpp.

    [2020-12-15]:   1. TODD: memory cache
                    2. test [2020-12-13] -> 1.2.
                    3. test [2020-12-09] -> 2.

    [2020-12-13]:   1. TODO: hide stack
                    2. TODO: delete debug info in dex file
                    3. deal with VmException and JavaException
                    4. don't test.

    [2020-12-09]:   1. TODO: stack in random address
                    2. have some bugs, eg: logcat is different every time
                        and may be too much log

    [2020-12-06]:   1. TODO: add code cache
                    2. fixed ip some bugs about vmc::tmp.

    [2020-12-03]:   1. TODO: add vm interpreter
                    2. TODO: test base vmp's App
                    3. only test sample opcode.

    [2020-11-23]:   1. TODO: change java method to native method
                    2. finish init VmMethodContext.

    [2020-11-15]:   1. TODO: test vm application name
                    2. TODO: main vm code about setup system
                    3. TODO: change java method to native method
                    4. think more and choose one.

    [2020-11-04]:   1. TODO: test parsing
                    2. TODO: write to new dex file
                    3. TODO: test my dex file by apktool
                    4. TODO: change AndroidManifest.xml
                    5. TODO: design a new format of data file
                    6. TODO: main shell step

    [2020-10-30]:   1. parse all data but no testing 
                    2. TODO: don't parse some data struct

    [2020-10-29]:   add test App and vm App

    [2020-10-29]:   init master; parse dex file
