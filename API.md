# SCM-Gabbiani OPC DA Agent - API Reference

## Overview

This document provides detailed API reference for the Ga1Agent OPC DA server components.

## Namespace Structure

```
OPCDA::          # OPC DA server components
  - OPCServer    # Main server class
  - OPCGroup     # Group management
  - OPCItem      # Item definitions
  - OPCItemValue # Data values

Gabbiani::       # Device interface
  - GabbianiDevice  # Hardware communication
  - DeviceTag       # Tag definitions

OPCService::     # Windows Service wrapper
  - Ga1AgentService # Service management
```

---

## OPCDA Namespace

### OPCServer Class

Main OPC DA server implementation.

#### Constructor
```cpp
OPCServer();
```

#### Methods

##### Initialize
```cpp
bool Initialize();
```
Initializes the OPC server and connects to the device.

**Returns**: `true` on success, `false` on failure

**Example**:
```cpp
OPCDA::OPCServer server;
if (server.Initialize()) {
    // Server ready
}
```

##### Shutdown
```cpp
bool Shutdown();
```
Shuts down the server and disconnects from the device.

**Returns**: `true` on success

##### CreateGroup
```cpp
DWORD CreateGroup(const std::wstring& groupName, const OPCGroupState& state);
```
Creates a new OPC group.

**Parameters**:
- `groupName`: Name of the group
- `state`: Initial group state

**Returns**: Group handle (DWORD)

**Example**:
```cpp
OPCDA::OPCGroupState state;
state.updateRate = 1000;  // 1 second
state.active = TRUE;

DWORD handle = server.CreateGroup(L"MyGroup", state);
```

##### RemoveGroup
```cpp
bool RemoveGroup(DWORD groupHandle);
```
Removes an OPC group.

**Parameters**:
- `groupHandle`: Handle of the group to remove

**Returns**: `true` on success

##### AddItems
```cpp
bool AddItems(DWORD groupHandle, 
              const std::vector<OPCItem>& items,
              std::vector<DWORD>& serverHandles);
```
Adds items to a group.

**Parameters**:
- `groupHandle`: Target group handle
- `items`: Vector of items to add
- `serverHandles`: Output vector of server handles

**Returns**: `true` on success

**Example**:
```cpp
std::vector<OPCDA::OPCItem> items;
OPCDA::OPCItem item;
item.itemID = L"Temperature";
item.clientHandle = 1;
items.push_back(item);

std::vector<DWORD> handles;
server.AddItems(groupHandle, items, handles);
```

##### ReadItems
```cpp
bool ReadItems(DWORD groupHandle,
               const std::vector<DWORD>& serverHandles,
               std::vector<OPCItemValue>& values,
               DWORD source);
```
Reads item values.

**Parameters**:
- `groupHandle`: Group handle
- `serverHandles`: Server handles of items to read
- `values`: Output vector of values
- `source`: `OPC_DS_CACHE` or `OPC_DS_DEVICE`

**Returns**: `true` on success

**Example**:
```cpp
std::vector<OPCDA::OPCItemValue> values;
server.ReadItems(groupHandle, handles, values, OPC_DS_DEVICE);

for (const auto& value : values) {
    if (value.error == S_OK) {
        // Use value.value (VARIANT)
        // Check value.quality
    }
}
```

##### WriteItems
```cpp
bool WriteItems(DWORD groupHandle,
                const std::vector<DWORD>& serverHandles,
                const std::vector<VARIANT>& values);
```
Writes values to items.

**Parameters**:
- `groupHandle`: Group handle
- `serverHandles`: Server handles of items to write
- `values`: Values to write

**Returns**: `true` on success

##### GetStatus
```cpp
OPCServerStatus GetStatus() const;
```
Gets server status information.

**Returns**: `OPCServerStatus` structure

**Example**:
```cpp
auto status = server.GetStatus();
wprintf(L"Server: %s\n", status.vendorInfo.c_str());
wprintf(L"State: %d\n", status.serverState);
wprintf(L"Groups: %d\n", status.groupCount);
```

##### BrowseItems
```cpp
std::vector<std::wstring> BrowseItems(const std::wstring& itemPath);
```
Browses available items.

**Parameters**:
- `itemPath`: Path to browse (empty for root)

**Returns**: Vector of item names

---

## Gabbiani Namespace

### GabbianiDevice Class

SCM-Gabbiani device interface.

#### Constructor
```cpp
GabbianiDevice();
```

#### Methods

##### Connect
```cpp
bool Connect(const std::wstring& portName, DWORD baudRate);
```
Connects to the device.

**Parameters**:
- `portName`: COM port (e.g., L"COM1")
- `baudRate`: Baud rate (e.g., 9600)

**Returns**: `true` on success

**Example**:
```cpp
Gabbiani::GabbianiDevice device;
if (device.Connect(L"COM1", 9600)) {
    // Connected
}
```

##### Disconnect
```cpp
bool Disconnect();
```
Disconnects from the device.

**Returns**: `true` on success

##### IsConnected
```cpp
bool IsConnected() const;
```
Checks if device is connected.

**Returns**: `true` if connected

##### RegisterTag
```cpp
bool RegisterTag(const DeviceTag& tag);
```
Registers a new tag.

**Parameters**:
- `tag`: Tag definition

**Returns**: `true` on success

**Example**:
```cpp
Gabbiani::DeviceTag tag;
tag.tagName = L"Temperature";
tag.dataType = VT_R4;
tag.address = 0x1000;
tag.scaleFactor = 0.1;
tag.offset = 0.0;

device.RegisterTag(tag);
```

##### ReadTag
```cpp
bool ReadTag(const std::wstring& tagName, VARIANT& value, WORD& quality);
```
Reads a tag value.

**Parameters**:
- `tagName`: Name of the tag
- `value`: Output value (VARIANT)
- `quality`: Output quality

**Returns**: `true` on success

**Example**:
```cpp
VARIANT value;
WORD quality;
VariantInit(&value);

if (device.ReadTag(L"Temperature", value, quality)) {
    if (quality == OPC_QUALITY_GOOD) {
        float temp = value.fltVal;
    }
}
VariantClear(&value);
```

##### WriteTag
```cpp
bool WriteTag(const std::wstring& tagName, const VARIANT& value);
```
Writes a tag value.

**Parameters**:
- `tagName`: Name of the tag
- `value`: Value to write

**Returns**: `true` on success

---

## Data Structures

### OPCItem
```cpp
struct OPCItem {
    std::wstring itemID;        // Item identifier
    std::wstring accessPath;    // Access path
    DWORD clientHandle;         // Client handle
    DWORD serverHandle;         // Server handle
    VARTYPE dataType;           // Data type
    DWORD accessRights;         // Access rights
    bool isActive;              // Active state
};
```

### OPCItemValue
```cpp
struct OPCItemValue {
    DWORD serverHandle;         // Server handle
    VARIANT value;              // Data value
    WORD quality;               // Quality (0-255)
    FILETIME timestamp;         // Timestamp
    HRESULT error;              // Error code
};
```

### OPCGroupState
```cpp
struct OPCGroupState {
    DWORD updateRate;           // Update rate (ms)
    BOOL active;                // Active state
    LONG timeBias;              // Time bias
    FLOAT deadband;             // Deadband (%)
    LCID localeID;              // Locale ID
    DWORD clientGroup;          // Client group handle
    DWORD serverGroup;          // Server group handle
};
```

### OPCServerStatus
```cpp
struct OPCServerStatus {
    FILETIME startTime;         // Start time
    FILETIME currentTime;       // Current time
    FILETIME lastUpdateTime;    // Last update time
    OPCSERVERSTATE serverState; // Server state
    DWORD groupCount;           // Number of groups
    DWORD bandwidth;            // Bandwidth
    WORD majorVersion;          // Major version
    WORD minorVersion;          // Minor version
    WORD buildNumber;           // Build number
    std::wstring vendorInfo;    // Vendor information
};
```

### DeviceTag
```cpp
struct DeviceTag {
    std::wstring tagName;       // Tag name
    std::wstring description;   // Description
    VARTYPE dataType;           // Data type
    DWORD address;              // Memory address
    double scaleFactor;         // Scale factor
    double offset;              // Offset
    std::wstring units;         // Engineering units
};
```

---

## Constants

### OPC Quality Flags
```cpp
#define OPC_QUALITY_GOOD        0xC0
#define OPC_QUALITY_BAD         0x00
#define OPC_QUALITY_UNCERTAIN   0x40
```

### OPC Access Rights
```cpp
#define OPC_READABLE            0x01
#define OPC_WRITABLE            0x02
```

### OPC Data Source
```cpp
#define OPC_DS_CACHE            0x01
#define OPC_DS_DEVICE           0x02
```

### OPC Server State
```cpp
enum OPCSERVERSTATE {
    OPC_STATUS_RUNNING = 1,
    OPC_STATUS_FAILED = 2,
    OPC_STATUS_NOCONFIG = 3,
    OPC_STATUS_SUSPENDED = 4,
    OPC_STATUS_TEST = 5,
    OPC_STATUS_COMM_FAULT = 6
};
```

### VARIANT Types
```cpp
VT_R4    // 32-bit float
VT_R8    // 64-bit double
VT_I2    // 16-bit signed integer
VT_I4    // 32-bit signed integer
VT_BOOL  // Boolean
```

---

## Usage Examples

### Complete Example: Reading Multiple Tags

```cpp
#include "include/opc_server.h"

int main() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    // Create and initialize server
    OPCDA::OPCServer server;
    if (!server.Initialize()) {
        return 1;
    }
    
    // Create group
    OPCDA::OPCGroupState groupState;
    groupState.updateRate = 1000;
    groupState.active = TRUE;
    DWORD groupHandle = server.CreateGroup(L"MyGroup", groupState);
    
    // Add items
    std::vector<OPCDA::OPCItem> items;
    OPCDA::OPCItem temp, pressure;
    temp.itemID = L"Temperature";
    temp.clientHandle = 1;
    pressure.itemID = L"Pressure";
    pressure.clientHandle = 2;
    items.push_back(temp);
    items.push_back(pressure);
    
    std::vector<DWORD> serverHandles;
    server.AddItems(groupHandle, items, serverHandles);
    
    // Read values
    std::vector<OPCDA::OPCItemValue> values;
    server.ReadItems(groupHandle, serverHandles, values, OPC_DS_DEVICE);
    
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i].error == S_OK) {
            wprintf(L"%s = %.2f\n", 
                    items[i].itemID.c_str(),
                    values[i].value.fltVal);
        }
    }
    
    // Cleanup
    server.Shutdown();
    CoUninitialize();
    return 0;
}
```

---

## Error Codes

### HRESULT Values
- `S_OK (0x00000000)`: Success
- `E_FAIL (0x80004005)`: Unspecified failure
- `E_INVALIDARG (0x80070057)`: Invalid argument
- `E_OUTOFMEMORY (0x8007000E)`: Out of memory

### Device Status
- `DEVICE_CONNECTED (0)`: Connected
- `DEVICE_DISCONNECTED (1)`: Disconnected
- `DEVICE_ERROR (2)`: Error state
- `DEVICE_INITIALIZING (3)`: Initializing

---

## Thread Safety

All public methods are thread-safe. Internal synchronization uses critical sections.

**Note**: When using callbacks, ensure proper thread synchronization in client code.

---

## Performance Considerations

- **Update Rate**: Minimum recommended: 100ms
- **Item Count**: Tested up to 1000 items per group
- **Group Count**: Tested up to 100 groups
- **Read Performance**: ~10ms per read cycle (device dependent)
- **Write Performance**: ~20ms per write cycle (device dependent)

---

## Compatibility

- **OPC Specification**: OPC DA 2.0/3.0
- **Windows**: 7, 8, 10, 11 (32-bit and 64-bit)
- **COM**: Thread-safe COM initialization required
- **Character Set**: Unicode (WCHAR)
