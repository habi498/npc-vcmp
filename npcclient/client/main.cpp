/*
    Copyright (C) 2022  habi

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <string>
#include <algorithm>
#include "tclap/CmdLine.h"
#include "main.h"
using namespace TCLAP;
using namespace std;
void start_consoleinput();
bool CreateFolder(const char* name);
CFunctions* m_pFunctions;
CPlugins* m_pPlugins;
bool bStdoutRedirected = false;
uint32_t configvalue = CONFIG_RESTORE_WEAPON_ON_SKIN_CHANGE | CONFIG_SYNC_ON_PLAYER_STREAMIN;
bool bDownloadStore = false;
std::string store_download_location;
bool bManualSpawn = false;
#define NPC_DIR "npcscripts"
#define NPC_PLUGINS_DIR "npcscripts/plugins"
//NPC_LOGS_DIR Warning: CreateDirectoryA will create only last folder 
//in path.
#define NPC_LOGS_DIR "npcscripts/logs"
#include <iostream>
#include <limits.h>   // For PATH_MAX
#ifdef LINUX
#include <unistd.h>   // POSIX header for getcwd()
#include <time.h>
long GetTickCount()
{
    struct timespec ts;
    long theTick = 0U;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    theTick = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}
#endif

#ifdef WIN32
    bool bShutdownSignal = false;
    static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType);
#include <direct.h>
#include <windows.h>
#define PATH_MAX MAX_PATH
#endif
void convertBackslashToForwardSlash(std::string& path);
int main(int argc, char** argv) {
    
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX)) {
    }
    else {
        printf("Error. could not get current working directory");
        exit(0);
    }

    std:string strloc = std::string(buffer);
    //printf("%s\n", strloc.c_str());
    //Get appdata location
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        strloc = std::string(appdata) + "\\VCMP\\04beta\\store";
    }
    else {
        strloc = "";
    }
#endif
    
    // Wrap everything in a try block.  Do this every time,
    // because exceptions will be thrown for problems.
    try {
        // Define the command line object.
        CmdLine cmd("VCMP-Non Player Characters v1.8.beta4 (16.Oct.2024)", ' ', "0.1b",false);

        // Define a value argument and add it to the command line.
        ValueArg<string> hostnameArg("h", "hostname", "IP address of host", false, "127.0.0.1",
                                 "string");
        ValueArg<int> portArg("p", "port", "Port to connect to", false, 8192,
            "integer");
        ValueArg<string> npcnameArg("n", "name", "Name of the NPC", true, "NPC",
            "string");
        ValueArg<string> fileArg("m", "scriptfile", "Squirrel Script file to be used", false, "",
            "string");
        ValueArg<string> passwdArg("z", "password", "Password of the server to connect", false, "",
            "string");
        ValueArg<string> pluginArg("q", "plugins", "List of plugins to be loaded", false, "",
            "string");
        
        ValueArg<string> LocationArg("l", "location", "The position(x,y,z), angle, skin, weapon and class to spawn \"x__ y__ z__ a__ s_ w_ c_\"", false, "",
            "string");
        MultiArg<string> scriptArg("w", "params", "The params to be passed to script", false, 
            "string");
        
        SwitchArg consoleInputSwitch("c", "consoleinput", "Use Console Input", cmd, false);
        ValueArg<string> execArg("e", "compilestring", "The string to be compiled by squirrel and executed on startup", false, "", "string");
        SwitchArg logfileSwitch("f", "logfile", "Logs stdout to file",cmd,false);
        SwitchArg downloadStoreSwitch("d", "download-store", "Get the store files to appdata/vcmp",cmd,false);
        ValueArg<string> storeLocationArg("D", "Store-Location", "The location to download store files. Use with -d switch", false, strloc.c_str(), "string");
        SwitchArg manualSpawn("M", "Manual-Spawn", "Disables automatic-spawn of npc", cmd, false);
        cmd.add(hostnameArg);
        cmd.add(portArg);
        cmd.add(npcnameArg);
        cmd.add(fileArg);
        cmd.add(passwdArg);
        cmd.add(LocationArg);
        cmd.add(scriptArg);
        cmd.add(pluginArg); 
        cmd.add(execArg);
        cmd.add(storeLocationArg);
        // Parse the args.
        cmd.parse(argc, argv);
        // Get the value parsed by each arg.
        string hostname = hostnameArg.getValue();
        int port = portArg.getValue();
        
        std::string npcname= npcnameArg.getValue();
        std::string npcscript = fileArg.getValue();
        std::string password = passwdArg.getValue();
        std::string location = LocationArg.getValue();
        std::vector<string> params=scriptArg.getValue();
        std::string execstring = execArg.getValue();
        store_download_location = storeLocationArg.getValue();
        convertBackslashToForwardSlash(store_download_location);
        if (store_download_location != "")
            store_download_location += "/";//otherwise it will go to C:\ folder
        store_download_location +=  hostname + "-" + std::to_string(port)+"/";
        bDownloadStore = downloadStoreSwitch.getValue();
        if (storeLocationArg.isSet() && !bDownloadStore)
        {
            printf("Activating -d switch since -D was used\n");
            bDownloadStore = true;
        }
        if (bDownloadStore)
            printf("Store download location: %s\n", store_download_location.c_str());
        if (manualSpawn.isSet())
        {
            bManualSpawn = true;
        }
        TimersInit();
        std::string scriptpath = "";
        if (npcscript.length() > 0)
            scriptpath = std::string(NPC_DIR + std::string("/") + npcscript);
        bool success = StartSquirrel(scriptpath.c_str(), location, params, execstring);
        if (success)
        {
#if WIN32
            SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#endif
            bool bUseConsoleInput = consoleInputSwitch.getValue();
            if (bUseConsoleInput)
                start_consoleinput();
            bool bWriteLog=logfileSwitch.getValue();
            if (bWriteLog)
            {
                //Create logs folder if not exist
                if (!CreateFolder(NPC_DIR))
                {
                    printf("Error in creating npcscripts directory for logs: %s\n", NPC_DIR);
                    exit(0);
                }
                //Create logs folder if not exist
                if (!CreateFolder(NPC_LOGS_DIR))
                {
                    printf("Error in creating logs directory: %s\n", NPC_LOGS_DIR);
                    exit(0);
                }
                time_t rawtime;
                struct tm* timeinfo;
                char buffer[100];
                time(&rawtime);
                timeinfo = localtime(&rawtime);//
                size_t chars_copied=strftime(buffer, 50, "%F %I-%M-%S %p ", timeinfo); //2001-08-23 02-55-02 PM
                if (chars_copied == 0)
                {
                    printf("Error in creating logfile\n");
                    exit(0);
                }
                strcat(buffer, npcname.c_str());
                strcat(buffer, "-log.txt"); //50+24=74 +7=81 
                char filepath[140];
                sprintf(filepath, "%s/%s", NPC_LOGS_DIR, buffer);
                FILE* f=freopen(filepath, "w", stdout);
                if (f)bStdoutRedirected = true;
            }
            InitSquirrelExports();
            //Need to initialize CFunctions class, as CPlugins need it.
            
            m_pFunctions = new CFunctions();
            if (!m_pFunctions)
                exit(0);
            m_pPlugins = new CPlugins();
            if (!m_pPlugins)
                exit(0);
            if (pluginArg.getValue().length() > 0)
            {
                 m_pPlugins->LoadPlugins(NPC_PLUGINS_DIR, pluginArg.getValue());
            }
#ifdef WIN32
            /*White letters on black background*/
            HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE); 
            if (hstdout)
                SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
            int s= LoadScript(scriptpath.c_str(), params);
            if (s)
                ConnectToServer(hostname, port, npcname, password);
            else
                goto error;
        }
        else {
        error:
            printf("Error in loading script file. Does it exist?");
            exit(0);
        }
        
    } catch (ArgException &e)  // catch any exceptions
    {
        cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
    }
}
#ifdef WIN32
// Handler function will be called on separate thread!
/*Function Credit: https://asawicki.info/news_1465_handling_ctrlc_in_windows_console_application*/
static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType)
{
   switch (dwCtrlType)
    {
    case CTRL_C_EVENT: // Ctrl+C  
    case CTRL_BREAK_EVENT: // Ctrl+Break
    case CTRL_CLOSE_EVENT: // Closing the console window
    case CTRL_LOGOFF_EVENT: // User logs off. Passed only to services!
    case CTRL_SHUTDOWN_EVENT: // System is shutting down. Passed only to services!
        bShutdownSignal = true;
        Sleep(CYCLE_SLEEP*3);//for wait time oncycle.
    }

    // Return TRUE if handled this message, further handler functions won't be called.
    // Return FALSE to pass this message to further handlers until default handler calls ExitProcess().
    return FALSE;
}
#endif
#ifdef _WIN32
bool CreateFolder(const char* name)
{
    if (!CreateDirectoryA(name, NULL) &&
        ERROR_ALREADY_EXISTS != GetLastError()) {
        printf("%d", GetLastError()); return false;
    }
    return true;
}
#else

bool CreateFolder(const char* name)
{
    mode_t mode = 0755;
    int s = mkdir(name, mode);
    if (s == 0 || errno == EEXIST)return true;
    return false;
}
#endif
void convertBackslashToForwardSlash(std::string& path) {
    for (char& c : path) {
        if (c == '\\') {
            c = '/';
        }
    }
}