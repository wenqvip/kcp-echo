# kcp-echo
A echo server/client with KCP  
For better reading of kcp code, I also add some comments in it. Welcome to check them.

#### How to compile:
##### Linux: 
install gcc-8 or above  
compile with `make`  
##### Win:
open `./vsproject/mystorm.sln` with Visual Studio 2019 and compile
#### How to use:
##### Start server:  
`./kcp-echo -s 127.0.0.1 1060`  
##### Start client:  
`./kcp-echo -c 127.0.0.1 1060`  
##### You can also check the help:  
`./kcp-echo -h`  
