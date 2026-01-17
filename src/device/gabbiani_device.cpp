#include "../include/gabbiani_device.h"
#include <stdio.h>

namespace Gabbiani {

GabbianiDevice::GabbianiDevice() 
    : hComPort_(INVALID_HANDLE_VALUE), status_(DEVICE_DISCONNECTED), 
      lastError_(0), baudRate_(9600), timeout_(1000) {
    InitializeCriticalSection(&csLock_);
}

GabbianiDevice::~GabbianiDevice() {
    Disconnect();
    DeleteCriticalSection(&csLock_);
}

bool GabbianiDevice::Connect(const std::wstring& portName, DWORD baudRate) {
    EnterCriticalSection(&csLock_);
    
    if (status_ == DEVICE_CONNECTED) {
        LeaveCriticalSection(&csLock_);
        return true;
    }
    
    status_ = DEVICE_INITIALIZING;
    portName_ = portName;
    baudRate_ = baudRate;
    
    // Open COM port
    hComPort_ = CreateFileW(
        portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (hComPort_ == INVALID_HANDLE_VALUE) {
        lastError_ = GetLastError();
        status_ = DEVICE_ERROR;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Configure COM port
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(hComPort_, &dcb)) {
        lastError_ = GetLastError();
        CloseHandle(hComPort_);
        hComPort_ = INVALID_HANDLE_VALUE;
        status_ = DEVICE_ERROR;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    dcb.BaudRate = baudRate_;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    
    if (!SetCommState(hComPort_, &dcb)) {
        lastError_ = GetLastError();
        CloseHandle(hComPort_);
        hComPort_ = INVALID_HANDLE_VALUE;
        status_ = DEVICE_ERROR;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Set timeouts
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = timeout_;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = timeout_;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    
    if (!SetCommTimeouts(hComPort_, &timeouts)) {
        lastError_ = GetLastError();
        CloseHandle(hComPort_);
        hComPort_ = INVALID_HANDLE_VALUE;
        status_ = DEVICE_ERROR;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Initialize device-specific communication
    if (!InitializeDevice()) {
        CloseHandle(hComPort_);
        hComPort_ = INVALID_HANDLE_VALUE;
        status_ = DEVICE_ERROR;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    status_ = DEVICE_CONNECTED;
    LeaveCriticalSection(&csLock_);
    return true;
}

bool GabbianiDevice::Disconnect() {
    EnterCriticalSection(&csLock_);
    
    if (hComPort_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hComPort_);
        hComPort_ = INVALID_HANDLE_VALUE;
    }
    
    status_ = DEVICE_DISCONNECTED;
    LeaveCriticalSection(&csLock_);
    return true;
}

bool GabbianiDevice::RegisterTag(const DeviceTag& tag) {
    EnterCriticalSection(&csLock_);
    tags_[tag.tagName] = tag;
    LeaveCriticalSection(&csLock_);
    return true;
}

bool GabbianiDevice::UnregisterTag(const std::wstring& tagName) {
    EnterCriticalSection(&csLock_);
    auto it = tags_.find(tagName);
    if (it != tags_.end()) {
        tags_.erase(it);
        LeaveCriticalSection(&csLock_);
        return true;
    }
    LeaveCriticalSection(&csLock_);
    return false;
}

std::vector<std::wstring> GabbianiDevice::GetTagList() const {
    std::vector<std::wstring> tagList;
    EnterCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    for (const auto& pair : tags_) {
        tagList.push_back(pair.first);
    }
    LeaveCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    return tagList;
}

bool GabbianiDevice::GetTagInfo(const std::wstring& tagName, DeviceTag& tag) const {
    EnterCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    auto it = tags_.find(tagName);
    if (it != tags_.end()) {
        tag = it->second;
        LeaveCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
        return true;
    }
    LeaveCriticalSection(const_cast<CRITICAL_SECTION*>(&csLock_));
    return false;
}

bool GabbianiDevice::ReadTag(const std::wstring& tagName, VARIANT& value, WORD& quality) {
    EnterCriticalSection(&csLock_);
    
    auto it = tags_.find(tagName);
    if (it == tags_.end()) {
        quality = OPC_QUALITY_BAD;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    const DeviceTag& tag = it->second;
    
    if (status_ != DEVICE_CONNECTED) {
        quality = OPC_QUALITY_BAD;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Read raw data from device
    BYTE buffer[256];
    DWORD bytesToRead = 4; // Assuming 4 bytes for most data types
    
    if (!ReadFromDevice(tag.address, buffer, bytesToRead)) {
        quality = OPC_QUALITY_BAD;
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Convert to VARIANT
    value = ConvertRawToVariant(buffer, tag.dataType, tag.scaleFactor, tag.offset);
    quality = OPC_QUALITY_GOOD;
    
    LeaveCriticalSection(&csLock_);
    return true;
}

bool GabbianiDevice::WriteTag(const std::wstring& tagName, const VARIANT& value) {
    EnterCriticalSection(&csLock_);
    
    auto it = tags_.find(tagName);
    if (it == tags_.end()) {
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    const DeviceTag& tag = it->second;
    
    if (status_ != DEVICE_CONNECTED) {
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Convert VARIANT to raw data
    BYTE buffer[256];
    if (!ConvertVariantToRaw(value, buffer, tag.dataType, tag.scaleFactor, tag.offset)) {
        LeaveCriticalSection(&csLock_);
        return false;
    }
    
    // Write raw data to device
    DWORD bytesToWrite = 4;
    bool result = WriteToDevice(tag.address, buffer, bytesToWrite);
    
    LeaveCriticalSection(&csLock_);
    return result;
}

bool GabbianiDevice::ReadMultipleTags(const std::vector<std::wstring>& tagNames, 
                                      std::vector<OPCDA::OPCItemValue>& values) {
    values.clear();
    
    for (const auto& tagName : tagNames) {
        OPCDA::OPCItemValue itemValue;
        if (ReadTag(tagName, itemValue.value, itemValue.quality)) {
            itemValue.error = S_OK;
        } else {
            itemValue.error = E_FAIL;
            itemValue.quality = OPC_QUALITY_BAD;
        }
        values.push_back(itemValue);
    }
    
    return true;
}

std::wstring GabbianiDevice::GetDeviceInfo() const {
    wchar_t info[512];
    swprintf(info, 512, L"SCM-Gabbiani 32bit Device\nPort: %s\nBaud: %d\nStatus: %d\nTags: %d",
             portName_.c_str(), baudRate_, status_, (int)tags_.size());
    return std::wstring(info);
}

bool GabbianiDevice::InitializeDevice() {
    // Send initialization command to SCM-Gabbiani device
    // This is device-specific and would need actual protocol implementation
    
    // For now, simulate successful initialization
    Sleep(100);
    return true;
}

bool GabbianiDevice::ReadFromDevice(DWORD address, BYTE* buffer, DWORD size) {
    if (hComPort_ == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Build read command for SCM-Gabbiani protocol
    // Protocol format: [STX][CMD][ADDR_H][ADDR_L][LEN][ETX]
    BYTE command[8];
    command[0] = 0x02; // STX
    command[1] = 0x52; // 'R' - Read command
    command[2] = (BYTE)((address >> 8) & 0xFF);
    command[3] = (BYTE)(address & 0xFF);
    command[4] = (BYTE)size;
    command[5] = 0x03; // ETX
    
    DWORD bytesWritten;
    if (!WriteFile(hComPort_, command, 6, &bytesWritten, NULL)) {
        lastError_ = GetLastError();
        return false;
    }
    
    // Read response
    BYTE response[256];
    DWORD bytesRead;
    if (!ReadFile(hComPort_, response, size + 4, &bytesRead, NULL)) {
        lastError_ = GetLastError();
        return false;
    }
    
    // Verify response and extract data
    if (bytesRead >= size + 4 && response[0] == 0x02 && response[bytesRead-1] == 0x03) {
        memcpy(buffer, &response[2], size);
        return true;
    }
    
    return false;
}

bool GabbianiDevice::WriteToDevice(DWORD address, const BYTE* buffer, DWORD size) {
    if (hComPort_ == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Build write command for SCM-Gabbiani protocol
    BYTE command[256];
    command[0] = 0x02; // STX
    command[1] = 0x57; // 'W' - Write command
    command[2] = (BYTE)((address >> 8) & 0xFF);
    command[3] = (BYTE)(address & 0xFF);
    command[4] = (BYTE)size;
    memcpy(&command[5], buffer, size);
    command[5 + size] = 0x03; // ETX
    
    DWORD bytesWritten;
    if (!WriteFile(hComPort_, command, 6 + size, &bytesWritten, NULL)) {
        lastError_ = GetLastError();
        return false;
    }
    
    // Read acknowledgment
    BYTE ack[4];
    DWORD bytesRead;
    if (!ReadFile(hComPort_, ack, 4, &bytesRead, NULL)) {
        lastError_ = GetLastError();
        return false;
    }
    
    // Verify ACK
    return (bytesRead >= 3 && ack[0] == 0x02 && ack[1] == 0x06);
}

VARIANT GabbianiDevice::ConvertRawToVariant(const BYTE* raw, VARTYPE dataType, double scale, double offset) {
    VARIANT var;
    VariantInit(&var);
    
    switch (dataType) {
        case VT_I2: {
            short value = *(short*)raw;
            var.vt = VT_I2;
            var.iVal = (short)(value * scale + offset);
            break;
        }
        case VT_I4: {
            long value = *(long*)raw;
            var.vt = VT_I4;
            var.lVal = (long)(value * scale + offset);
            break;
        }
        case VT_R4: {
            float value = *(float*)raw;
            var.vt = VT_R4;
            var.fltVal = (float)(value * scale + offset);
            break;
        }
        case VT_R8: {
            double value = *(double*)raw;
            var.vt = VT_R8;
            var.dblVal = value * scale + offset;
            break;
        }
        case VT_BOOL: {
            var.vt = VT_BOOL;
            var.boolVal = raw[0] ? VARIANT_TRUE : VARIANT_FALSE;
            break;
        }
        default:
            var.vt = VT_R4;
            var.fltVal = (float)(*(float*)raw * scale + offset);
            break;
    }
    
    return var;
}

bool GabbianiDevice::ConvertVariantToRaw(const VARIANT& value, BYTE* raw, VARTYPE dataType, double scale, double offset) {
    switch (dataType) {
        case VT_I2: {
            short val = (short)((value.iVal - offset) / scale);
            memcpy(raw, &val, sizeof(short));
            break;
        }
        case VT_I4: {
            long val = (long)((value.lVal - offset) / scale);
            memcpy(raw, &val, sizeof(long));
            break;
        }
        case VT_R4: {
            float val = (float)((value.fltVal - offset) / scale);
            memcpy(raw, &val, sizeof(float));
            break;
        }
        case VT_R8: {
            double val = (value.dblVal - offset) / scale;
            memcpy(raw, &val, sizeof(double));
            break;
        }
        case VT_BOOL: {
            raw[0] = value.boolVal ? 1 : 0;
            break;
        }
        default:
            return false;
    }
    
    return true;
}

} // namespace Gabbiani
