# kcp-echo ![C/C++ CI](https://github.com/wenqvip/kcp-echo/workflows/C/C++%20CI/badge.svg?branch=master)[![Build Status](https://drone.wenqlive.cn:7200/api/badges/wenqvip/kcp-echo/status.svg)](https://drone.wenqlive.cn:7200/wenqvip/kcp-echo)

A echo server/client with KCP  
For better reading of code, I also add some comments in it. Welcome to check them.

## How to compile

- Ubuntu:  
  
    ```shell
    sudo apt install -y clang
    make
    ```

- Windows:  

    open `./vsproject/mystorm.sln` with Visual Studio 2019 and compile with c++17  

## How to use

- Quick run (default IP is 127.0.0.1, port 1060):
    server: `./kcp-echo -s`  
    client: `./kcp-echo -c`

- You can check RTT of every packet:
    server: `./kcp-echo -s -d`  
    client: `./kcp-echo -c -d`

- Perform a ping pong test in one minute, after 1 minute, client will tell you how many times the ping-pong action is performed. The average RTT can be calculated with `60000 / ping-pong-times`
    server: `./kcp-echo -s -p`  
    client: `./kcp-echo -c -p`

- Other options:  
    show help: `./kcp-echo -h`  
    show kcp log: `-l`  
