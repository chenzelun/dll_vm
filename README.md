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
  + [ ] hide stack
  + [ ] stack in random address
  + [ ] cancel JNI call
  + [ ] delete debug info in dex file
  + [ ] test App's profile
  
  
  Additional:
  + [ ] zip my data file
  + [ ] opt reg memory
  + [ ] register key functions dynamically and hide the map
  

## git log
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
