#ifndef OPC_SERVICE_H
#define OPC_SERVICE_H

#include <windows.h>
#include <string>
#include "opc_server.h"

namespace OPCService {

// Service configuration
#define SERVICE_NAME        L"GabbianiOPCDA"
#define SERVICE_DISPLAY     L"SCM-Gabbiani OPC DA Agent Service"
#define SERVICE_DESCRIPTION L"OPC DA Server for SCM-Gabbiani 32bit devices"

// Service class
class Ga1AgentService {
public:
    Ga1AgentService();
    ~Ga1AgentService();
    
    // Service control
    static bool Install();
    static bool Uninstall();
    static bool Start();
    static bool Stop();
    
    // Service entry point
    static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD ctrlCode);
    
    // Run as console application
    static bool RunConsole();
    
private:
    // Service status
    static SERVICE_STATUS serviceStatus_;
    static SERVICE_STATUS_HANDLE serviceStatusHandle_;
    static HANDLE stopEvent_;
    static OPCDA::OPCServer* opcServer_;
    
    // Helper methods
    static void UpdateServiceStatus(DWORD currentState, DWORD exitCode = NO_ERROR);
    static void ServiceWorkerThread();
    static void LogMessage(const wchar_t* message);
};

} // namespace OPCService

#endif // OPC_SERVICE_H
