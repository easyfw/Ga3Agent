#ifndef GABBIANI_DEVICE_H
#define GABBIANI_DEVICE_H

#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include "opcda_types.h"

namespace Gabbiani {

// Device communication status
enum DeviceStatus {
    DEVICE_CONNECTED = 0,
    DEVICE_DISCONNECTED = 1,
    DEVICE_ERROR = 2,
    DEVICE_INITIALIZING = 3
};

// Tag data structure for SCM-Gabbiani device
struct DeviceTag {
    std::wstring tagName;
    std::wstring description;
    VARTYPE dataType;
    DWORD address;
    double scaleFactor;
    double offset;
    std::wstring units;
    
    DeviceTag() : dataType(VT_R4), address(0), scaleFactor(1.0), offset(0.0) {}
};

// Main device interface class
class GabbianiDevice {
public:
    GabbianiDevice();
    ~GabbianiDevice();
    
    // Connection management
    bool Connect(const std::wstring& portName, DWORD baudRate);
    bool Disconnect();
    bool IsConnected() const { return status_ == DEVICE_CONNECTED; }
    DeviceStatus GetStatus() const { return status_; }
    
    // Tag management
    bool RegisterTag(const DeviceTag& tag);
    bool UnregisterTag(const std::wstring& tagName);
    std::vector<std::wstring> GetTagList() const;
    bool GetTagInfo(const std::wstring& tagName, DeviceTag& tag) const;
    
    // Data access
    bool ReadTag(const std::wstring& tagName, VARIANT& value, WORD& quality);
    bool WriteTag(const std::wstring& tagName, const VARIANT& value);
    bool ReadMultipleTags(const std::vector<std::wstring>& tagNames, 
                          std::vector<OPCDA::OPCItemValue>& values);
    
    // Device information
    std::wstring GetDeviceInfo() const;
    DWORD GetLastError() const { return lastError_; }
    
private:
    // Internal methods
    bool InitializeDevice();
    bool ReadFromDevice(DWORD address, BYTE* buffer, DWORD size);
    bool WriteToDevice(DWORD address, const BYTE* buffer, DWORD size);
    VARIANT ConvertRawToVariant(const BYTE* raw, VARTYPE dataType, double scale, double offset);
    bool ConvertVariantToRaw(const VARIANT& value, BYTE* raw, VARTYPE dataType, double scale, double offset);
    
    // Member variables
    HANDLE hComPort_;
    DeviceStatus status_;
    DWORD lastError_;
    std::map<std::wstring, DeviceTag> tags_;
    CRITICAL_SECTION csLock_;
    
    // Configuration
    std::wstring portName_;
    DWORD baudRate_;
    DWORD timeout_;
};

} // namespace Gabbiani

#endif // GABBIANI_DEVICE_H
