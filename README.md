# kcp-echo
A echo server/client with KCP

## How to compile:
Linux: `make`  
Win: open `./vsproject/mystorm.sln` with Visual Studio 2019 and compile

## How to use:
Start server:  
  `./kcp-echo -s 127.0.0.1 1060`  
Start client:  
  `./kcp-echo -c 127.0.0.1 1060`  
You can also check the help:  
  `./kcp-echo -h`  