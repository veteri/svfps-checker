#include <iostream>
#include <vector>
#include <numeric>
#include <iomanip>
#include <Windows.h>
#include "Process.h"

#define LEGIT 0x0
#define CHEATED 0x1

#define SCAN_CNT 3

static bool isSilent = false;
static HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);


void getStringBlocking(HANDLE processHandle, uintptr_t address, char* dest) { 
    char c = 0;
    int position = 0;
    do {
        ReadProcessMemory(processHandle, (void*)(address + position), &c, sizeof(c), nullptr);
        //Wait until game loaded the map
        if (position == 0 && c == 0) {
            Sleep(10);
            continue;
        }

        dest[position++] = c;

        if (c == 0) {
            break;
        }

    } while (true);
}

int isIngame(HANDLE processHandle, uintptr_t moduleBase) {
    uintptr_t isIngamePtr = moduleBase + 0x348710;
    int isIngame = 0;
    ReadProcessMemory(processHandle, (void*) isIngamePtr, &isIngame, sizeof(isIngame), 0);
    return isIngame;
}

void cleanResult(std::vector<int> *pSvFpsArray) {
    //Remove last the entries containing 0 because it possibly scanned in menu, resulting in sv_fps 0
    int removeCount = (int) std::round(pSvFpsArray->size() * 0.1f);
    if (removeCount < 1)
        removeCount = 1;

    for (unsigned int i = 0; i < removeCount; i++) {
        //Delete first
        pSvFpsArray->erase(pSvFpsArray->begin());
        //Delete last
        pSvFpsArray->pop_back();
    }
}

void performScan(HANDLE processHandle, uintptr_t moduleBaseAddress, std::vector<int> *pSvFpsArray) {

    system("cls");
    SetConsoleTextAttribute(consoleHandle, 15);
    std::cout << " ----------------------------" << std::endl;
    std::cout << "|     Collecting data...     |" << std::endl;
   

    //Address of iw3mp.exe image base + that offset = static pointer to server time
    uintptr_t serverTimePtr = moduleBaseAddress + 0x3446DC;

    //Print table header
    std::cout << " ----------------------------" << std::endl;
    std::cout << "|     Scan #   |   sv_fps    |" << std::endl;
    std::cout << "|----------------------------|" << std::endl;

    
    //Scan for as long as the demo is playing
    int i;
    for (i = 0; isIngame(processHandle, moduleBaseAddress); i++) {

        SetConsoleTextAttribute(consoleHandle, 15);

        //First scan
        int serverTimeFirstScan = 0;
        ReadProcessMemory(processHandle, (BYTE*)serverTimePtr, &serverTimeFirstScan, sizeof(serverTimeFirstScan), nullptr);
        Sleep(1000);

        //Second scan
        int serverTimeSecondScan = 0;
        ReadProcessMemory(processHandle, (BYTE*)serverTimePtr, &serverTimeSecondScan, sizeof(serverTimeSecondScan), nullptr);

        //Build difference ( = sv_fps )
        int svFps = serverTimeSecondScan - serverTimeFirstScan;

        //Print row
        std::cout << "|    " << std::setw(4) << i + 1 << "      |    " << std::setw(4) << svFps << "     |" << std::endl;
        std::cout << "|----------------------------|" << std::endl;

        //Save to array
        pSvFpsArray->push_back(svFps);
    }

    
   
}

void printResult(std::vector<int> svFpsArray, const char *playerName, const char *serverName) {

    int svFpsSum = std::accumulate(svFpsArray.begin(), svFpsArray.end(), 0);
    uint32_t averageSVFPS = (uint32_t)std::round(svFpsSum / (float) svFpsArray.size());


    std::cout << "| Player:  " << playerName << std::endl;
    std::cout << "| Server:  " << serverName  << std::endl;

    std::cout << "| Avg sv_fps: ";
    SetConsoleTextAttribute(consoleHandle, averageSVFPS == 20 ? 10 : 12);
    std::cout << averageSVFPS << std::endl;

    SetConsoleTextAttribute(consoleHandle, 15);
    std::cout << "| Verdict: ";

    if (averageSVFPS != 20) {
        SetConsoleTextAttribute(consoleHandle, 12);
        std::cout << "Cheated";
    }
    else {
        SetConsoleTextAttribute(consoleHandle, 10);
        std::cout << "Clean";
    }

    SetConsoleTextAttribute(consoleHandle, 15);
    std::cout << std::endl << "|============================|" << std::endl;
}

void analyzeDemo(HANDLE processHandle, uintptr_t moduleBase) {

    SetConsoleTextAttribute(consoleHandle, 14);
    std::cout << std::endl << "Waiting for demo..." << std::endl;
    while (!isIngame(processHandle, moduleBase)) {
        Sleep(10);
    }

    //Get name of demo performer
    char playerName[100] = { 0 };
    char serverName[100] = { 0 };
    getStringBlocking(processHandle, moduleBase + 0x43927C, playerName);
    getStringBlocking(processHandle, moduleBase + 0x34A948, serverName);


    //Scan sv_fps for duration of demo
    std::vector<int> svFpsArray;
    performScan(processHandle, moduleBase, &svFpsArray);

    //Remove invalid scan entries
    cleanResult(&svFpsArray);

    //Print verdict
    printResult(svFpsArray, playerName, serverName);
}

int main(int argc, char* argv[])
{

    HWND console = GetConsoleWindow();
    RECT r;
    GetWindowRect(console, &r);
    MoveWindow(console, r.left, r.top, 800, 700, TRUE);

    //Get Process Id of cod4
    DWORD processId = getProcessIdByName(L"iw3mp.exe");
    //std::cout << "Cod4 PID: " << processId << std::endl;

    //Get the Module Base Address of the executable
    uintptr_t moduleBase = GetModuleBaseAddress(processId, L"iw3mp.exe");

    //Get a process handle
    HANDLE processHandle = 0;
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, NULL, processId);

    //Main Menu
    SetConsoleTextAttribute(consoleHandle, 11);
    const char *intro3 = 
        "                                                                              \n\
                                               ,,,,,                                   \n\
                                     ,,,,,,,,,,,,,,,,,,,,,,,,,                         \n\
                                 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,                    \n\
                              ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,                 \n\
                           ,,,,,,,,,,,,,,,,,.          ,,,,,,,,,,,,,,,//*              \n\
                          ,,,,,,,,,,,,,,                      ,,,,,,,//////*           \n\
                       ,,,,,,,,,,,,             @               ,///////////           \n\
                      ,,,,,,,,,,,              @/(                 //////////*         \n\
                     ,,,,,,,,,,.              .//#%                 ///////////        \n\
                    ,,,,,,,,,,                ///##@                 ,/////////#       \n\
                    ,,,,,,,,,     &//********////##///////@@@%/@      //////////       \n\
                   ,,,,,,,,,,        @//////***//(/(%%@     @          /////////       \n\
                   ,,,,,,,,,,           @(////**/(///%      @          /////////       \n\
                   ,,,,,,,,,,            //***///%%@   #@   @          /////////       \n\
                   (,,,,,,,,,            /**/////%@   @#@   @          /////////       \n\
                    ,,,,,,,,,,          %*////@  @          ##       //////////        \n\
                     ,,,,,,,,,,        @*/@      @&&&&&&&   &&#      //////////        \n\
                     %,,,,,,,,,,      @@                @   @      ///////////         \n\
                       ,,,,,,,,,,,                      @@@@@    ////////////          \n\
                       %*,,,,,,,,////                         ,////////////            \n\
                    %%%%%%,,,,///////////,                ///////////////*             \n\
                  %%%%%%%%%%%//////////////////////////////////////////                \n\
               %%%%%%%%%%&&&&&&(////////////////////////////////////                   \n\
             %%%%%%%%%%&&&&&&&&&&@@*////////////////////////////#                      \n\
          %%%%%%%%%%&&&&&&&&&&           *////////////////                             \n\
        %%%%%%%%%%&&&&&&&&&&                                                           \n\
     %%%%%%%%%%&&&&&&&&&&                                                              \n\
   %%%%%%%%%%&&&&&&&&&&                                                                \n\
 %%%%%%%%%&&&&&&&&&&               ~   sv_fps checker v1.0   ~                         \n\
%%%%%%%%&&&&&&&&&&                                                                     \n\
 %%%%&&&&&&&&&&                                                                        \n\
  %&&&&&&&&&&                                                                          \n";


    std::cout << intro3 << std::endl << std::endl;
    

    do {
        analyzeDemo(processHandle, moduleBase);
    } while (true);

    return 0;
}