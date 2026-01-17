#ifndef OPC_SERVER_H
#define OPC_SERVER_H

#include <windows.h>
#include <map>
#include <vector>
#include <string>
#include "opcda_types.h"
#include "gabbiani_device.h"

namespace OPCDA {

// Forward declarations
class OPCGroup;

// Main OPC Server class
class OPCServer {
public:
    OPCServer();
    ~OPCServer();
    
    // Server lifecycle
    bool Initialize();
    bool Shutdown();
    bool IsRunning() const { return isRunning_; }
    
    // Group management
    DWORD CreateGroup(const std::wstring& groupName, const OPCGroupState& state);
    bool RemoveGroup(DWORD groupHandle);
    OPCGroup* GetGroup(DWORD groupHandle);
    std::vector<DWORD> GetGroupHandles() const;
    
    // Item management (delegated to groups)
    bool AddItems(DWORD groupHandle, const std::vector<OPCItem>& items, std::vector<DWORD>& serverHandles);
    bool RemoveItems(DWORD groupHandle, const std::vector<DWORD>& serverHandles);
    
    // Data access
    bool ReadItems(DWORD groupHandle, const std::vector<DWORD>& serverHandles, 
                   std::vector<OPCItemValue>& values, DWORD source);
    bool WriteItems(DWORD groupHandle, const std::vector<DWORD>& serverHandles,
                    const std::vector<VARIANT>& values);
    
    // Server status
    OPCServerStatus GetStatus() const;
    
    // Browse
    std::vector<std::wstring> BrowseItems(const std::wstring& itemPath);
    
    // Device access
    Gabbiani::GabbianiDevice* GetDevice() { return &device_; }
    
private:
    // Member variables
    bool isRunning_;
    DWORD nextGroupHandle_;
    std::map<DWORD, OPCGroup*> groups_;
    Gabbiani::GabbianiDevice device_;
    OPCServerStatus status_;
    CRITICAL_SECTION csLock_;
    
    // Helper methods
    DWORD GenerateGroupHandle();
};

// OPC Group class
class OPCGroup {
public:
    OPCGroup(DWORD handle, const std::wstring& name, const OPCGroupState& state, OPCServer* server);
    ~OPCGroup();
    
    // Group properties
    DWORD GetHandle() const { return handle_; }
    std::wstring GetName() const { return name_; }
    OPCGroupState GetState() const { return state_; }
    bool SetState(const OPCGroupState& state);
    
    // Item management
    bool AddItems(const std::vector<OPCItem>& items, std::vector<DWORD>& serverHandles);
    bool RemoveItems(const std::vector<DWORD>& serverHandles);
    std::vector<DWORD> GetItemHandles() const;
    
    // Data access
    bool ReadItems(const std::vector<DWORD>& serverHandles, std::vector<OPCItemValue>& values, DWORD source);
    bool WriteItems(const std::vector<DWORD>& serverHandles, const std::vector<VARIANT>& values);
    
private:
    DWORD handle_;
    std::wstring name_;
    OPCGroupState state_;
    OPCServer* server_;
    std::map<DWORD, OPCItem> items_;
    DWORD nextItemHandle_;
    CRITICAL_SECTION csLock_;
    
    DWORD GenerateItemHandle();
};

} // namespace OPCDA

#endif // OPC_SERVER_H
