#include "../include/opc_server.h"
#include <algorithm>

namespace OPCDA {

//==============================================================================
// OPCServer Implementation
//==============================================================================

OPCServer::OPCServer() : isRunning_(false), nextGroupHandle_(1000) {
    InitializeCriticalSection(&csLock_);
}

OPCServer::~OPCServer() {
    Shutdown();
    DeleteCriticalSection(&csLock_);
}

bool OPCServer::Initialize() {
    EnterCriticalSection(&csLock_);
    
    if (isRunning_) {
        LeaveCriticalSection(&csLock_);
        return true;
    }
    
    // Initialize device connection
    if (!device_.Connect(L"COM1", 9600)) {
        status_.serverState = OPC_STATUS_COMM_FAULT;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Register default tags for SCM-Gabbiani device
    Gabbiani::DeviceTag tag;
    
    // Temperature tag
    tag.tagName = L"Temperature";
    tag.description = L"Temperature sensor reading";
    tag.dataType = VT_R4;
    tag.address = 0x1000;
    tag.scaleFactor = 0.1;
    tag.offset = 0.0;
    tag.units = L"°C";
    device_.RegisterTag(tag);
    
    // Pressure tag
    tag.tagName = L"Pressure";
    tag.description = L"Pressure sensor reading";
    tag.dataType = VT_R4;
    tag.address = 0x1004;
    tag.scaleFactor = 0.01;
    tag.offset = 0.0;
    tag.units = L"bar";
    device_.RegisterTag(tag);
    
    // Flow tag
    tag.tagName = L"Flow";
    tag.description = L"Flow rate measurement";
    tag.dataType = VT_R4;
    tag.address = 0x1008;
    tag.scaleFactor = 0.1;
    tag.offset = 0.0;
    tag.units = L"l/min";
    device_.RegisterTag(tag);
    
    // Status tag
    tag.tagName = L"Status";
    tag.description = L"Device status";
    tag.dataType = VT_I4;
    tag.address = 0x100C;
    tag.scaleFactor = 1.0;
    tag.offset = 0.0;
    tag.units = L"";
    device_.RegisterTag(tag);
    
    // Alarm tag
    tag.tagName = L"Alarm";
    tag.description = L"Alarm state";
    tag.dataType = VT_BOOL;
    tag.address = 0x1010;
    tag.scaleFactor = 1.0;
    tag.offset = 0.0;
    tag.units = L"";
    device_.RegisterTag(tag);
    
    isRunning_ = true;
    status_.serverState = OPC_STATUS_RUNNING;
    GetSystemTimeAsFileTime(&status_.startTime);
    
    LeaveCriticalSection(&csLock_);
    return true;
}

bool OPCServer::Shutdown() {
    EnterCriticalSection(&csLock_);
    
    if (!isRunning_) {
        LeaveCriticalSection(&csLock_);
        return true;
    }
    
    // Remove all groups
    for (auto& pair : groups_) {
        delete pair.second;
    }
    groups_.clear();
    
    // Disconnect device
    device_.Disconnect();
    
    isRunning_ = false;
    status_.serverState = OPC_STATUS_SUSPENDED;
    
    LeaveCriticalSection(&csLock_);
    return true;
}

DWORD OPCServer::CreateGroup(const std::wstring& groupName, const OPCGroupState& state) {
    EnterCriticalSection(&csLock_);
    
    DWORD handle = GenerateGroupHandle();
    OPCGroup* group = new OPCGroup(handle, groupName, state, this);
    groups_[handle] = group;
    status_.groupCount = (DWORD)groups_.size();
    
    LeaveCriticalSection(&csLock_);
    return handle;
}

bool OPCServer::RemoveGroup(DWORD groupHandle) {
    EnterCriticalSection(&csLock_);
    
    auto it = groups_.find(groupHandle);
    if (it != groups_.end()) {
        delete it->second;
        groups_.erase(it);
        status_.groupCount = (DWORD)groups_.size();
        LeaveCriticalSection(&csLock_);
        return true;
    }
    
    LeaveCriticalSection(&csLock_);
    return false;
}

OPCGroup* OPCServer::GetGroup(DWORD groupHandle) {
    EnterCriticalSection(&csLock_);
    auto it = groups_.find(groupHandle);
    OPCGroup* group = (it != groups_.end()) ? it->second : nullptr;
    LeaveCriticalSection(&csLock_);
    return group;
}

std::vector<DWORD> OPCServer::GetGroupHandles() const {
    std::vector<DWORD> handles;
    EnterCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    for (const auto& pair : groups_) {
        handles.push_back(pair.first);
    }
    LeaveCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    return handles;
}

bool OPCServer::AddItems(DWORD groupHandle, const std::vector<OPCItem>& items, 
                         std::vector<DWORD>& serverHandles) {
    OPCGroup* group = GetGroup(groupHandle);
    if (!group) {
        return false;
    }
    return group->AddItems(items, serverHandles);
}

bool OPCServer::RemoveItems(DWORD groupHandle, const std::vector<DWORD>& serverHandles) {
    OPCGroup* group = GetGroup(groupHandle);
    if (!group) {
        return false;
    }
    return group->RemoveItems(serverHandles);
}

bool OPCServer::ReadItems(DWORD groupHandle, const std::vector<DWORD>& serverHandles,
                          std::vector<OPCItemValue>& values, DWORD source) {
    OPCGroup* group = GetGroup(groupHandle);
    if (!group) {
        return false;
    }
    return group->ReadItems(serverHandles, values, source);
}

bool OPCServer::WriteItems(DWORD groupHandle, const std::vector<DWORD>& serverHandles,
                           const std::vector<VARIANT>& values) {
    OPCGroup* group = GetGroup(groupHandle);
    if (!group) {
        return false;
    }
    return group->WriteItems(serverHandles, values);
}

OPCServerStatus OPCServer::GetStatus() const {
    EnterCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    OPCServerStatus status = status_;
    GetSystemTimeAsFileTime(&status.currentTime);
    LeaveCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    return status;
}

std::vector<std::wstring> OPCServer::BrowseItems(const std::wstring& itemPath) {
    return device_.GetTagList();
}

DWORD OPCServer::GenerateGroupHandle() {
    return nextGroupHandle_++;
}

//==============================================================================
// OPCGroup Implementation
//==============================================================================

OPCGroup::OPCGroup(DWORD handle, const std::wstring& name, const OPCGroupState& state, OPCServer* server)
    : handle_(handle), name_(name), state_(state), server_(server), nextItemHandle_(2000) {
    InitializeCriticalSection(&csLock_);
}

OPCGroup::~OPCGroup() {
    DeleteCriticalSection(&csLock_);
}

bool OPCGroup::SetState(const OPCGroupState& state) {
    EnterCriticalSection(&csLock_);
    state_ = state;
    LeaveCriticalSection(&csLock_);
    return true;
}

bool OPCGroup::AddItems(const std::vector<OPCItem>& items, std::vector<DWORD>& serverHandles) {
    EnterCriticalSection(&csLock_);
    
    serverHandles.clear();
    for (const auto& item : items) {
        OPCItem newItem = item;
        newItem.serverHandle = GenerateItemHandle();
        items_[newItem.serverHandle] = newItem;
        serverHandles.push_back(newItem.serverHandle);
    }
    
    LeaveCriticalSection(&csLock_);
    return true;
}

bool OPCGroup::RemoveItems(const std::vector<DWORD>& serverHandles) {
    EnterCriticalSection(&csLock_);
    
    for (DWORD handle : serverHandles) {
        auto it = items_.find(handle);
        if (it != items_.end()) {
            items_.erase(it);
        }
    }
    
    LeaveCriticalSection(&csLock_);
    return true;
}

std::vector<DWORD> OPCGroup::GetItemHandles() const {
    std::vector<DWORD> handles;
    EnterCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    for (const auto& pair : items_) {
        handles.push_back(pair.first);
    }
    LeaveCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    return handles;
}

bool OPCGroup::ReadItems(const std::vector<DWORD>& serverHandles, 
                         std::vector<OPCItemValue>& values, DWORD source) {
    EnterCriticalSection(&csLock_);
    
    values.clear();
    Gabbiani::GabbianiDevice* device = server_->GetDevice();
    
    for (DWORD handle : serverHandles) {
        auto it = items_.find(handle);
        if (it != items_.end()) {
            OPCItemValue itemValue;
            itemValue.serverHandle = handle;
            
            if (device->ReadTag(it->second.itemID, itemValue.value, itemValue.quality)) {
                itemValue.error = S_OK;
                GetSystemTimeAsFileTime(&itemValue.timestamp);
            } else {
                itemValue.error = E_FAIL;
                itemValue.quality = OPC_QUALITY_BAD;
            }
            
            values.push_back(itemValue);
        }
    }
    
    LeaveCriticalSection(&csLock_);
    return !values.empty();
}

bool OPCGroup::WriteItems(const std::vector<DWORD>& serverHandles, 
                          const std::vector<VARIANT>& values) {
    if (serverHandles.size() != values.size()) {
        return false;
    }
    
    EnterCriticalSection(&csLock_);
    
    Gabbiani::GabbianiDevice* device = server_->GetDevice();
    bool allSuccess = true;
    
    for (size_t i = 0; i < serverHandles.size(); i++) {
        auto it = items_.find(serverHandles[i]);
        if (it != items_.end()) {
            if (!device->WriteTag(it->second.itemID, values[i])) {
                allSuccess = false;
            }
        } else {
            allSuccess = false;
        }
    }
    
    LeaveCriticalSection(&csLock_);
    return allSuccess;
}

DWORD OPCGroup::GenerateItemHandle() {
    return nextItemHandle_++;
}

} // namespace OPCDA
