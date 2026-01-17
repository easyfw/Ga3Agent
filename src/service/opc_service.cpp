#include "../include/opc_service.h"
#include <stdio.h>

namespace OPCService {

// Static member initialization
SERVICE_STATUS Ga1AgentService::serviceStatus_ = {0};
SERVICE_STATUS_HANDLE Ga1AgentService::serviceStatusHandle_ = NULL;
HANDLE Ga1AgentService::stopEvent_ = NULL;
OPCDA::OPCServer* Ga1AgentService::opcServer_ = nullptr;

Ga1AgentService::Ga1AgentService() {
}

Ga1AgentService::~Ga1AgentService() {
}

bool Ga1AgentService::Install() {
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scManager) {
        LogMessage(L"Failed to open Service Control Manager");
        return false;
    }
    
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(NULL, path, MAX_PATH)) {
        CloseServiceHandle(scManager);
        return false;
    }
    
    SC_HANDLE service = CreateServiceW(
        scManager,
        SERVICE_NAME,
        SERVICE_DISPLAY,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        path,
        NULL, NULL, NULL, NULL, NULL
    );
    
    if (!service) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS) {
            LogMessage(L"Service already exists");
        } else {
            LogMessage(L"Failed to create service");
        }
        CloseServiceHandle(scManager);
        return false;
    }
    
    // Set service description
    SERVICE_DESCRIPTIONW desc;
    desc.lpDescription = const_cast<LPWSTR>(SERVICE_DESCRIPTION);
    ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &desc);
    
    LogMessage(L"Service installed successfully");
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

bool Ga1AgentService::Uninstall() {
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        LogMessage(L"Failed to open Service Control Manager");
        return false;
    }
    
    SC_HANDLE service = OpenServiceW(scManager, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (!service) {
        LogMessage(L"Failed to open service");
        CloseServiceHandle(scManager);
        return false;
    }
    
    // Try to stop the service first
    SERVICE_STATUS status;
    ControlService(service, SERVICE_CONTROL_STOP, &status);
    Sleep(1000);
    
    // Delete the service
    if (!DeleteService(service)) {
        LogMessage(L"Failed to delete service");
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }
    
    LogMessage(L"Service uninstalled successfully");
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

bool Ga1AgentService::Start() {
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        LogMessage(L"Failed to open Service Control Manager");
        return false;
    }
    
    SC_HANDLE service = OpenServiceW(scManager, SERVICE_NAME, SERVICE_START);
    if (!service) {
        LogMessage(L"Failed to open service");
        CloseServiceHandle(scManager);
        return false;
    }
    
    if (!StartServiceW(service, 0, NULL)) {
        LogMessage(L"Failed to start service");
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }
    
    LogMessage(L"Service started successfully");
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

bool Ga1AgentService::Stop() {
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        LogMessage(L"Failed to open Service Control Manager");
        return false;
    }
    
    SC_HANDLE service = OpenServiceW(scManager, SERVICE_NAME, SERVICE_STOP);
    if (!service) {
        LogMessage(L"Failed to open service");
        CloseServiceHandle(scManager);
        return false;
    }
    
    SERVICE_STATUS status;
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
        LogMessage(L"Failed to stop service");
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }
    
    LogMessage(L"Service stopped successfully");
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

void WINAPI Ga1AgentService::ServiceMain(DWORD argc, LPWSTR* argv) {
    // Register service control handler
    serviceStatusHandle_ = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
    if (!serviceStatusHandle_) {
        return;
    }
    
    // Initialize service status
    ZeroMemory(&serviceStatus_, sizeof(serviceStatus_));
    serviceStatus_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus_.dwCurrentState = SERVICE_START_PENDING;
    UpdateServiceStatus(SERVICE_START_PENDING);
    
    // Create stop event
    stopEvent_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!stopEvent_) {
        UpdateServiceStatus(SERVICE_STOPPED, GetLastError());
        return;
    }
    
    // Start the service
    UpdateServiceStatus(SERVICE_RUNNING);
    LogMessage(L"Service started");
    
    // Run the service worker thread
    ServiceWorkerThread();
    
    // Service stopped
    CloseHandle(stopEvent_);
    UpdateServiceStatus(SERVICE_STOPPED);
    LogMessage(L"Service stopped");
}

void WINAPI Ga1AgentService::ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
        case SERVICE_CONTROL_STOP:
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            SetEvent(stopEvent_);
            break;
            
        case SERVICE_CONTROL_SHUTDOWN:
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            SetEvent(stopEvent_);
            break;
            
        case SERVICE_CONTROL_INTERROGATE:
            break;
            
        default:
            break;
    }
}

void Ga1AgentService::UpdateServiceStatus(DWORD currentState, DWORD exitCode) {
    serviceStatus_.dwCurrentState = currentState;
    serviceStatus_.dwWin32ExitCode = exitCode;
    serviceStatus_.dwWaitHint = 0;
    
    if (currentState == SERVICE_START_PENDING || currentState == SERVICE_STOP_PENDING) {
        serviceStatus_.dwControlsAccepted = 0;
        serviceStatus_.dwWaitHint = 3000;
    } else {
        serviceStatus_.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }
    
    SetServiceStatus(serviceStatusHandle_, &serviceStatus_);
}

void Ga1AgentService::ServiceWorkerThread() {
    // Create and initialize OPC Server
    opcServer_ = new OPCDA::OPCServer();
    
    if (!opcServer_->Initialize()) {
        LogMessage(L"Failed to initialize OPC Server");
        delete opcServer_;
        opcServer_ = nullptr;
        return;
    }
    
    LogMessage(L"OPC Server initialized successfully");
    
    // Wait for stop event
    while (WaitForSingleObject(stopEvent_, 1000) == WAIT_TIMEOUT) {
        // Service is running
        // Could add periodic health checks or maintenance tasks here
    }
    
    // Shutdown OPC Server
    if (opcServer_) {
        opcServer_->Shutdown();
        delete opcServer_;
        opcServer_ = nullptr;
    }
    
    LogMessage(L"OPC Server shutdown complete");
}

bool Ga1AgentService::RunConsole() {
    wprintf(L"SCM-Gabbiani OPC DA Agent Service - Console Mode\n");
    wprintf(L"================================================\n\n");
    
    // Create and initialize OPC Server
    OPCDA::OPCServer server;
    
    wprintf(L"Initializing OPC Server...\n");
    if (!server.Initialize()) {
        wprintf(L"ERROR: Failed to initialize OPC Server\n");
        return false;
    }
    
    wprintf(L"OPC Server started successfully\n");
    wprintf(L"Server: %s\n", server.GetStatus().vendorInfo.c_str());
    wprintf(L"Version: %d.%d.%d\n", 
            server.GetStatus().majorVersion,
            server.GetStatus().minorVersion,
            server.GetStatus().buildNumber);
    
    // Display available tags
    auto tags = server.BrowseItems(L"");
    wprintf(L"\nAvailable Tags:\n");
    for (const auto& tag : tags) {
        wprintf(L"  - %s\n", tag.c_str());
    }
    
    wprintf(L"\nPress Ctrl+C to stop...\n\n");
    
    // Create a test group and add items
    OPCDA::OPCGroupState groupState;
    groupState.updateRate = 1000;
    groupState.active = TRUE;
    
    DWORD groupHandle = server.CreateGroup(L"TestGroup", groupState);
    wprintf(L"Created group: TestGroup (handle: %d)\n", groupHandle);
    
    // Add items to the group
    std::vector<OPCDA::OPCItem> items;
    for (const auto& tagName : tags) {
        OPCDA::OPCItem item;
        item.itemID = tagName;
        item.clientHandle = (DWORD)items.size() + 1;
        item.isActive = true;
        items.push_back(item);
    }
    
    std::vector<DWORD> serverHandles;
    if (server.AddItems(groupHandle, items, serverHandles)) {
        wprintf(L"Added %d items to group\n", (int)serverHandles.size());
    }
    
    // Main loop - read values periodically
    int loopCount = 0;
    while (loopCount < 60) { // Run for 60 seconds in console mode
        Sleep(2000);
        
        std::vector<OPCDA::OPCItemValue> values;
        if (server.ReadItems(groupHandle, serverHandles, values, OPC_DS_DEVICE)) {
            wprintf(L"\n[%d] Tag Values:\n", loopCount);
            for (size_t i = 0; i < values.size() && i < items.size(); i++) {
                wprintf(L"  %s = ", items[i].itemID.c_str());
                
                if (values[i].error == S_OK) {
                    switch (values[i].value.vt) {
                        case VT_R4:
                            wprintf(L"%.2f", values[i].value.fltVal);
                            break;
                        case VT_I4:
                            wprintf(L"%d", values[i].value.lVal);
                            break;
                        case VT_BOOL:
                            wprintf(L"%s", values[i].value.boolVal ? L"TRUE" : L"FALSE");
                            break;
                        default:
                            wprintf(L"(unknown type)");
                            break;
                    }
                    wprintf(L" (Quality: 0x%02X)\n", values[i].quality);
                } else {
                    wprintf(L"ERROR\n");
                }
            }
        }
        
        loopCount++;
    }
    
    // Cleanup
    wprintf(L"\nShutting down OPC Server...\n");
    server.Shutdown();
    wprintf(L"Shutdown complete\n");
    
    return true;
}

void Ga1AgentService::LogMessage(const wchar_t* message) {
    // Log to Windows Event Log
    HANDLE eventLog = RegisterEventSourceW(NULL, SERVICE_NAME);
    if (eventLog) {
        const wchar_t* strings[1] = { message };
        ReportEventW(eventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, strings, NULL);
        DeregisterEventSource(eventLog);
    }
    
    // Also write to console if available
    wprintf(L"[Ga1Agent] %s\n", message);
}

} // namespace OPCService
