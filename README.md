# Ga1Agent - SCM-Gabbiani 32bit OPC DA Agent Service

## Overview

Ga1Agent is a Windows-based OPC DA (Data Access) server implementation for SCM-Gabbiani 32-bit industrial devices. It provides a standardized OPC DA interface for real-time data acquisition from Gabbiani hardware using serial communication (RS232/RS485).

## Features

- **OPC DA 2.0/3.0 Compliant**: Standard OPC Data Access implementation
- **32-bit Architecture**: Optimized for legacy systems and OPC DA compatibility
- **Serial Communication**: RS232/RS485 support for SCM-Gabbiani devices
- **Windows Service**: Runs as a background Windows service
- **Console Mode**: Testing and debugging mode
- **Real-time Data**: Synchronous and asynchronous data access
- **Configurable Tags**: Easy tag configuration via INI file
- **Quality Indicators**: OPC quality flags and timestamps
- **Multiple Groups**: Support for multiple OPC groups and items

## System Requirements

- **Operating System**: Windows 7/8/10/11 (32-bit or 64-bit)
- **Hardware**: SCM-Gabbiani 32-bit device with serial interface
- **Serial Port**: COM port (RS232/RS485)
- **Visual Studio**: 2015 or later (for compilation)
- **CMake**: 3.10 or later (optional, for CMake builds)

## Building

### Using CMake (Recommended)

```bash
mkdir build
cd build
cmake -A Win32 ..
cmake --build . --config Release
```

### Using Visual Studio

1. Open `Ga1Agent.vcxproj` in Visual Studio
2. Select **Win32** platform (for 32-bit build)
3. Build the solution (Release or Debug)

The executable will be generated in `bin/Release/Ga1Agent.exe` or `bin/Debug/Ga1Agent.exe`.

## Installation

### As Windows Service

1. Build the project
2. Copy the executable and configuration files to your installation directory
3. Run as Administrator:

```cmd
Ga1Agent.exe install
```

4. Start the service:

```cmd
Ga1Agent.exe start
```

Or use Windows Services Manager (services.msc) to start "SCM-Gabbiani OPC DA Agent Service".

### Console Mode (for Testing)

Run the executable with the console parameter:

```cmd
Ga1Agent.exe console
```

This will run the OPC server in the foreground with diagnostic output.

## Configuration

### Tag Configuration

Edit `config/tags.ini` to define data items for your SCM-Gabbiani device:

```ini
[Temperature]
Description = Temperature sensor reading
DataType = VT_R4
Address = 0x1000
Scale = 0.1
Offset = 0.0
Units = °C
```

**Supported Data Types:**
- `VT_R4` - 32-bit floating point
- `VT_R8` - 64-bit floating point
- `VT_I2` - 16-bit integer
- `VT_I4` - 32-bit integer
- `VT_BOOL` - Boolean

### Serial Port Configuration

The default COM port is `COM1` with baud rate `9600`. To change this, modify the connection parameters in `src/server/opc_server.cpp`:

```cpp
device_.Connect(L"COM1", 9600);
```

Supported baud rates: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200

## Usage

### Service Commands

```cmd
# Install the service
Ga1Agent.exe install

# Uninstall the service
Ga1Agent.exe uninstall

# Start the service
Ga1Agent.exe start

# Stop the service
Ga1Agent.exe stop

# Run in console mode
Ga1Agent.exe console
```

### OPC Client Connection

Connect your OPC client (e.g., Kepware, Matrikon, etc.) to:

- **Server Name**: `GabbianiOPCDA`
- **ProgID**: `Gabbiani.OPCServer.1` (if COM registered)
- **CLSID**: (depends on COM registration)

### Available Tags

Default tags (can be customized in `config/tags.ini`):

- `Temperature` - Temperature sensor (°C)
- `Pressure` - Pressure sensor (bar)
- `Flow` - Flow rate (l/min)
- `Status` - Device status word
- `Alarm` - Alarm state (boolean)
- `SetPoint` - Temperature setpoint (°C)
- `ControlMode` - Control mode (0=Manual, 1=Auto)
- `RunTime` - Total runtime (hours)

## Architecture

```
Ga1Agent/
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── server/
│   │   └── opc_server.cpp          # OPC DA server implementation
│   ├── device/
│   │   └── gabbiani_device.cpp     # SCM-Gabbiani device interface
│   └── service/
│       └── opc_service.cpp         # Windows service wrapper
├── include/
│   ├── opcda_types.h               # OPC DA data types and structures
│   ├── opc_server.h                # OPC server class definitions
│   ├── gabbiani_device.h           # Device interface definitions
│   └── opc_service.h               # Service class definitions
├── config/
│   └── tags.ini                    # Tag configuration file
├── CMakeLists.txt                  # CMake build configuration
└── Ga1Agent.vcxproj                # Visual Studio project file
```

## Protocol

The SCM-Gabbiani communication protocol uses a simple request/response format over RS232/RS485:

**Read Command:**
```
[STX][R][ADDR_H][ADDR_L][LEN][ETX]
```

**Write Command:**
```
[STX][W][ADDR_H][ADDR_L][LEN][DATA...][ETX]
```

- `STX` = 0x02 (Start of Text)
- `ETX` = 0x03 (End of Text)
- `R` = 0x52 (Read command)
- `W` = 0x57 (Write command)
- `ADDR_H/L` = Address high/low bytes
- `LEN` = Data length
- `DATA` = Data bytes

## Troubleshooting

### Service won't start

1. Check COM port availability (Device Manager)
2. Verify serial connection to SCM-Gabbiani device
3. Check Windows Event Log for error messages
4. Run in console mode to see detailed error messages

### OPC Client can't connect

1. Ensure the service is running
2. Check Windows Firewall settings
3. Verify DCOM configuration (dcomcnfg)
4. Enable local/remote access for OPC DA

### No data from device

1. Verify serial cable connection
2. Check COM port settings (baud rate, parity, etc.)
3. Confirm device is powered on and operational
4. Use console mode to monitor communication

## Development

### Adding New Tags

1. Edit `config/tags.ini` to define the new tag
2. Restart the service or application
3. The tag will be available for OPC clients to subscribe

### Modifying Device Protocol

Edit `src/device/gabbiani_device.cpp` and update:
- `ReadFromDevice()` - Read command format
- `WriteToDevice()` - Write command format
- Protocol-specific parsing and checksum

### Extending OPC Functionality

- **Add groups**: Modify `src/server/opc_server.cpp`
- **Add callbacks**: Implement `IOPCDataCallback` interface
- **Add browsing**: Extend `BrowseItems()` method

## License

This project is provided as-is for integration with SCM-Gabbiani industrial devices.

## Support

For issues related to:
- **OPC DA specification**: Refer to OPC Foundation documentation
- **SCM-Gabbiani devices**: Contact Gabbiani technical support
- **This implementation**: Check the GitHub repository issues

## Version History

- **v1.0.0** - Initial release
  - OPC DA server implementation
  - SCM-Gabbiani device support
  - Windows service functionality
  - Console testing mode

## Contributing

Contributions are welcome! Please submit pull requests or open issues for:
- Bug fixes
- Feature enhancements
- Documentation improvements
- Protocol extensions

---

**Note**: This is a 32-bit application designed for maximum compatibility with OPC DA clients and legacy systems. For new installations, ensure you're using 32-bit OPC clients or appropriate WOW64 settings.