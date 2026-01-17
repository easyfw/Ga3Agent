#ifndef OPCDA_TYPES_H
#define OPCDA_TYPES_H

#include <windows.h>
#include <string>
#include <vector>

// OPC DA Quality Flags
#define OPC_QUALITY_GOOD            0xC0
#define OPC_QUALITY_BAD             0x00
#define OPC_QUALITY_UNCERTAIN       0x40

// OPC Access Rights
#define OPC_READABLE                0x01
#define OPC_WRITABLE                0x02

// OPC Data Source
#define OPC_DS_CACHE                0x01
#define OPC_DS_DEVICE               0x02

// OPC Browse Type
#define OPC_BRANCH                  0x01
#define OPC_LEAF                    0x02
#define OPC_FLAT                    0x03

namespace OPCDA {

// OPC Item Definition
struct OPCItem {
    std::wstring itemID;
    std::wstring accessPath;
    DWORD clientHandle;
    DWORD serverHandle;
    VARTYPE dataType;
    DWORD accessRights;
    bool isActive;
    
    OPCItem() : clientHandle(0), serverHandle(0), dataType(VT_EMPTY), 
                accessRights(OPC_READABLE), isActive(true) {}
};

// OPC Item Value
struct OPCItemValue {
    DWORD serverHandle;
    VARIANT value;
    WORD quality;
    FILETIME timestamp;
    HRESULT error;
    
    OPCItemValue() : serverHandle(0), quality(OPC_QUALITY_GOOD), error(S_OK) {
        VariantInit(&value);
        GetSystemTimeAsFileTime(&timestamp);
    }
    
    ~OPCItemValue() {
        VariantClear(&value);
    }
};

// OPC Group State
struct OPCGroupState {
    DWORD updateRate;
    BOOL active;
    LONG timeBias;
    FLOAT deadband;
    LCID localeID;
    DWORD clientGroup;
    DWORD serverGroup;
    
    OPCGroupState() : updateRate(1000), active(TRUE), timeBias(0), 
                      deadband(0.0f), localeID(0), clientGroup(0), serverGroup(0) {}
};

// OPC Server Status
struct OPCServerStatus {
    FILETIME startTime;
    FILETIME currentTime;
    FILETIME lastUpdateTime;
    OPCSERVERSTATE serverState;
    DWORD groupCount;
    DWORD bandwidth;
    WORD majorVersion;
    WORD minorVersion;
    WORD buildNumber;
    std::wstring vendorInfo;
    
    OPCServerStatus() : serverState(OPC_STATUS_RUNNING), groupCount(0), 
                        bandwidth(0), majorVersion(1), minorVersion(0), buildNumber(1) {
        GetSystemTimeAsFileTime(&startTime);
        GetSystemTimeAsFileTime(&currentTime);
        GetSystemTimeAsFileTime(&lastUpdateTime);
        vendorInfo = L"SCM-Gabbiani OPC DA Server 32bit";
    }
};

// Server State Enumeration
enum OPCSERVERSTATE {
    OPC_STATUS_RUNNING = 1,
    OPC_STATUS_FAILED = 2,
    OPC_STATUS_NOCONFIG = 3,
    OPC_STATUS_SUSPENDED = 4,
    OPC_STATUS_TEST = 5,
    OPC_STATUS_COMM_FAULT = 6
};

} // namespace OPCDA

#endif // OPCDA_TYPES_H
