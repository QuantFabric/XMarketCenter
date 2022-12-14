#include "Logger.h"
#include "XMarketCenter.h"
#include <stdio.h>

void printHelp()
{
    printf("Usage:  XMarketCenter -d -f ~/config.yml\n");
    printf("\t-f: Config File Path\n");
    printf("\t-a: Account\n");
    printf("\t-L: Plugin So path\n");
    printf("\t-d: log debug mode, print debug log\n");
    printf("\t-h: print help information\n");
}

int main(int argc, char *argv[])
{
    std::string configPath = "./config.yml";
    std::string Account;
    int ch;
    std::string soPath;
    bool check = false;
    bool debug = false;
    while ((ch = getopt(argc, argv, "f:a:L:dh")) != -1)
    {
        switch (ch)
        {
        case 'f':
            configPath = optarg;
            break;
        case 'a':
            Account = optarg;
            break;
        case 'L':
            soPath = optarg;
            break;
        case 'd':
            debug = true;
            break;
        case 'h':
        case '?':
        case ':':
        default:
            printHelp();
            exit(-1);
            break;
        }
    }
    std::string app_log_path;
    char* p = getenv("APP_LOG_PATH");
    if(p == NULL)
    {
        app_log_path = "./log/";
    }
    else
    {
        app_log_path = p;
    }
    Utils::gLogger = Utils::Singleton<Utils::Logger>::GetInstance();
    Utils::gLogger->setLogPath(app_log_path, "XMarketCenter");
    Utils::gLogger->Init();
    Utils::gLogger->setDebugLevel(debug);
    std::string cmd;
    for(int i = 0; i < argc; i++)
    {
        cmd += (std::string(argv[i]) + " ");
    }
    // start Market Center
    XMarketCenter engine;
    engine.LoadConfig(configPath.c_str());
    engine.SetCommand(cmd);
    engine.LoadMarketGateWay(soPath);
    engine.Run();

    return 0;
}
