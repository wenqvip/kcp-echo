#include "ikcp.h"
#include "argparse.hpp"
#include "storm.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("kcp test");
    parser.add_argument("-s", "--server").help("server mode");
    parser.add_argument("-c", "--client").help("client mode");
    parser.add_argument("host")
        .help("remote ip, default: 127.0.0.1")
        .default_value("127.0.0.1");
    parser.add_argument("port")
        .help("remote port, default: 1060")
        .default_value(1060);

    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << parser;
        exit(0);
    }

    Storm storm;
    storm.CreateSession(parser.get<const char*>("host"), parser.get<int>("port"));

    return 0;
}