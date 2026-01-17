#include "include/opc_service.h"
#include <stdio.h>
#include <wchar.h>

void PrintUsage() {
    wprintf(L"SCM-Gabbiani OPC DA Agent Service - 32bit\n");
    wprintf(L"==========================================\n\n");
    wprintf(L"Usage:\n");
    wprintf(L"  Ga1Agent.exe install   - Install the service\n");
    wprintf(L"  Ga1Agent.exe uninstall - Uninstall the service\n");
    wprintf(L"  Ga1Agent.exe start     - Start the service\n");
    wprintf(L"  Ga1Agent.exe stop      - Stop the service\n");
    wprintf(L"  Ga1Agent.exe console   - Run in console mode (for testing)\n");
    wprintf(L"  Ga1Agent.exe           - Run as service (called by SCM)\n");
    wprintf(L"\n");
}

int wmain(int argc, wchar_t* argv[]) {
    // Initialize COM
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    if (argc > 1) {
        if (_wcsicmp(argv[1], L"install") == 0) {
            if (OPCService::Ga1AgentService::Install()) {
                wprintf(L"Service installed successfully\n");
                wprintf(L"Use 'Ga1Agent.exe start' to start the service\n");
                CoUninitialize();
                return 0;
            } else {
                wprintf(L"Failed to install service\n");
                CoUninitialize();
                return 1;
            }
        }
        else if (_wcsicmp(argv[1], L"uninstall") == 0) {
            if (OPCService::Ga1AgentService::Uninstall()) {
                wprintf(L"Service uninstalled successfully\n");
                CoUninitialize();
                return 0;
            } else {
                wprintf(L"Failed to uninstall service\n");
                CoUninitialize();
                return 1;
            }
        }
        else if (_wcsicmp(argv[1], L"start") == 0) {
            if (OPCService::Ga1AgentService::Start()) {
                wprintf(L"Service started successfully\n");
                CoUninitialize();
                return 0;
            } else {
                wprintf(L"Failed to start service\n");
                CoUninitialize();
                return 1;
            }
        }
        else if (_wcsicmp(argv[1], L"stop") == 0) {
            if (OPCService::Ga1AgentService::Stop()) {
                wprintf(L"Service stopped successfully\n");
                CoUninitialize();
                return 0;
            } else {
                wprintf(L"Failed to stop service\n");
                CoUninitialize();
                return 1;
            }
        }
        else if (_wcsicmp(argv[1], L"console") == 0) {
            bool result = OPCService::Ga1AgentService::RunConsole();
            CoUninitialize();
            return result ? 0 : 1;
        }
        else {
            PrintUsage();
            CoUninitialize();
            return 1;
        }
    }
    
    // Run as service (no arguments)
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<LPWSTR>(OPCService::SERVICE_NAME), OPCService::Ga1AgentService::ServiceMain },
        { NULL, NULL }
    };
    
    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        DWORD error = GetLastError();
        if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            // Not running as service, show usage
            PrintUsage();
            CoUninitialize();
            return 1;
        }
    }
    
    CoUninitialize();
    return 0;
}
