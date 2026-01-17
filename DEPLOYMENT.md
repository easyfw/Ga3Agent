# Deployment Guide for SCM-Gabbiani OPC DA Agent

## Overview

This guide covers deploying the Ga1Agent OPC DA server on Windows systems for production use.

## Pre-Installation Checklist

- [ ] Windows 7 or later (32-bit or 64-bit)
- [ ] Serial port available (COM port)
- [ ] SCM-Gabbiani device connected
- [ ] Administrator privileges
- [ ] OPC clients ready for testing

## Installation Steps

### 1. Prepare Installation Files

Create an installation folder (e.g., `C:\Program Files (x86)\Ga1Agent\`):

```
C:\Program Files (x86)\Ga1Agent\
├── Ga1Agent.exe
├── config\
│   └── tags.ini
└── README.md
```

### 2. Configure Serial Port

1. Open Device Manager
2. Find your COM port under "Ports (COM & LPT)"
3. Note the port number (e.g., COM1, COM2)
4. Right-click > Properties > Port Settings
5. Verify settings match device requirements:
   - Baud rate: 9600 (default)
   - Data bits: 8
   - Parity: None
   - Stop bits: 1
   - Flow control: None

### 3. Configure Tags

Edit `config\tags.ini` to match your SCM-Gabbiani device:

```ini
[Temperature]
Description = Temperature sensor reading
DataType = VT_R4
Address = 0x1000
Scale = 0.1
Offset = 0.0
Units = °C
```

**Important**: Verify memory addresses match your device specification.

### 4. Install Windows Service

Open Command Prompt as Administrator:

```cmd
cd "C:\Program Files (x86)\Ga1Agent"
Ga1Agent.exe install
```

Expected output:
```
Service installed successfully
Use 'Ga1Agent.exe start' to start the service
```

### 5. Configure Service Startup

#### Automatic Startup (Recommended)

```cmd
sc config GabbianiOPCDA start= auto
```

#### Manual Startup

```cmd
sc config GabbianiOPCDA start= demand
```

#### Delayed Automatic Startup (if COM port needs time)

```cmd
sc config GabbianiOPCDA start= delayed-auto
```

### 6. Start the Service

```cmd
Ga1Agent.exe start
```

Or using Windows Services:
1. Press Win+R, type `services.msc`, press Enter
2. Find "SCM-Gabbiani OPC DA Agent Service"
3. Right-click > Start

### 7. Verify Service Status

Check Event Viewer:
1. Press Win+R, type `eventvwr.msc`, press Enter
2. Navigate to Windows Logs > Application
3. Look for "GabbianiOPCDA" events

Expected log messages:
- "Service started"
- "OPC Server initialized successfully"

## Testing

### Test with Console Mode (Before Installing Service)

```cmd
Ga1Agent.exe console
```

This will:
- Display startup information
- List available tags
- Read tag values every 2 seconds
- Run for 60 seconds (test mode)

Verify output shows:
- "OPC Server started successfully"
- Tag values updating
- No errors

### Test with OPC Client

Use any OPC DA client (Kepware, Matrikon OPC Explorer, etc.):

1. **Launch OPC client**
2. **Connect to server**:
   - Server name: `GabbianiOPCDA`
   - Node: `localhost` (or machine name)
3. **Browse available tags**
4. **Add tags to client** (e.g., Temperature, Pressure, Flow)
5. **Monitor values** - should update in real-time
6. **Test write** (if supported by your device)

## DCOM Configuration

For remote OPC access, configure DCOM:

### 1. Run DCOM Config

```cmd
dcomcnfg
```

### 2. Configure Component Services

1. Navigate to: Component Services > Computers > My Computer
2. Right-click "My Computer" > Properties
3. **Default Properties** tab:
   - Enable Distributed COM: Yes
   - Default Authentication Level: Connect
   - Default Impersonation Level: Identify

### 3. Configure OPC Server (if COM registered)

1. Navigate to: DCOM Config > [Your OPC Server]
2. Right-click > Properties
3. **General** tab: Authentication Level: Default
4. **Location** tab: Check "Run application on this computer"
5. **Security** tab:
   - Launch and Activation Permissions: Customize > Add users
   - Access Permissions: Customize > Add users
6. **Identity** tab: Select "Interactive User"

### 4. Windows Firewall

Add firewall rules for OPC communication:

```cmd
netsh advfirewall firewall add rule name="OPC DA Server" dir=in action=allow program="C:\Program Files (x86)\Ga1Agent\Ga1Agent.exe" enable=yes
```

Or use Windows Firewall GUI:
1. Control Panel > Windows Defender Firewall > Advanced Settings
2. Inbound Rules > New Rule
3. Program > Browse to Ga1Agent.exe
4. Allow the connection
5. Apply to all profiles

## Troubleshooting

### Service won't start

**Check Event Viewer** for specific error codes.

**Common issues**:

1. **COM Port in use**
   ```cmd
   # List COM ports
   mode
   
   # Kill process using COM port
   # (Identify in Resource Monitor)
   ```

2. **Permissions issue**
   ```cmd
   # Grant service account COM port access
   # Run as Administrator
   icacls "C:\Program Files (x86)\Ga1Agent" /grant "NT AUTHORITY\SYSTEM:(OI)(CI)F"
   ```

3. **Missing dependencies**
   - Install Visual C++ Redistributable 2015-2019
   - Download from Microsoft

### No data from device

1. **Verify physical connection**
   - Check cable
   - Verify device is powered on
   - LED indicators active

2. **Test COM port**
   ```cmd
   # Use terminal emulator (e.g., PuTTY, Tera Term)
   # Connect to COM port
   # Send test commands manually
   ```

3. **Check tag configuration**
   - Verify addresses in tags.ini
   - Confirm data types
   - Check scaling factors

### OPC Client can't connect

1. **Verify service is running**
   ```cmd
   sc query GabbianiOPCDA
   ```

2. **Check network connectivity** (remote access)
   ```cmd
   ping [server-machine-name]
   ```

3. **Test DCOM configuration**
   - Use OpcEnum.exe to browse servers
   - Check Windows Event Log for DCOM errors

4. **Disable antivirus temporarily** (testing only)
   - Some antivirus software blocks OPC communication

## Performance Tuning

### Update Rate

Edit group update rate in OPC client (default: 1000ms)
- Lower values = faster updates, higher CPU usage
- Higher values = slower updates, lower CPU usage

### Device Communication Timeout

Modify in `src/device/gabbiani_device.cpp`:

```cpp
timeout_ = 1000; // 1000ms default
```

### Thread Priority

For real-time requirements, set service to high priority:

```cmd
sc config GabbianiOPCDA start= auto
# Then in Registry:
# HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\GabbianiOPCDA
# Add DWORD: "Priority" = 2 (High)
```

## Backup and Recovery

### Backup Configuration

```cmd
# Backup tags configuration
copy "C:\Program Files (x86)\Ga1Agent\config\tags.ini" "C:\Backup\"

# Backup registry (if COM registered)
reg export HKEY_CLASSES_ROOT\CLSID\{your-clsid} "C:\Backup\opc_registry.reg"
```

### Recovery

```cmd
# Stop service
Ga1Agent.exe stop

# Restore configuration
copy "C:\Backup\tags.ini" "C:\Program Files (x86)\Ga1Agent\config\"

# Restart service
Ga1Agent.exe start
```

## Uninstallation

```cmd
# Stop and uninstall service
Ga1Agent.exe stop
Ga1Agent.exe uninstall

# Remove installation directory
rmdir /s /q "C:\Program Files (x86)\Ga1Agent"

# Remove firewall rule (if added)
netsh advfirewall firewall delete rule name="OPC DA Server"
```

## Security Recommendations

1. **Service Account**: Run as dedicated service account (not SYSTEM)
2. **File Permissions**: Restrict write access to config files
3. **Network**: Use firewall rules to limit OPC access
4. **Audit**: Enable logging and monitor Event Viewer
5. **Updates**: Keep Windows and dependencies updated

## Monitoring

### Windows Performance Monitor

Monitor OPC server performance:

```cmd
perfmon
```

Add counters:
- Process > % Processor Time > Ga1Agent
- Process > Working Set > Ga1Agent
- Serial Port activity

### Event Log Monitoring

Set up Event Log alerts for critical errors:

```cmd
wevtutil qe Application /q:"*[System[Provider[@Name='GabbianiOPCDA']]]" /f:text
```

## Support Information

- **Product**: Ga1Agent v1.0.0
- **Device**: SCM-Gabbiani 32-bit
- **Protocol**: OPC DA 2.0/3.0
- **Platform**: Windows 7+ (32-bit compatible)

For technical support, refer to:
- README.md - General information
- BUILD.md - Build instructions
- GitHub Issues - Bug reports and feature requests
