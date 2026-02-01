# Quick Start Guide - Ga1Agent OPC DA Server

## For Developers

### 1. Get the Code
```bash
git clone https://github.com/easyfw/Ga1Agent.git
cd Ga1Agent
```

### 2. Build (Choose One Method)

**Using CMake:**
```cmd
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -A Win32 ..
cmake --build . --config Release
```

**Using Visual Studio:**
```cmd
# Open Ga1Agent.vcxproj in Visual Studio
# Select Win32 platform, Release configuration
# Build > Build Solution (F7)
```

**Using nmake:**
```cmd
# Open Developer Command Prompt for VS
nmake /f Makefile.win
```

### 3. Test
```cmd
# Run in console mode
bin\Release\Ga1Agent.exe console

# Or from build directory
build\bin\Ga1Agent.exe console
```

Expected output:
```
SCM-Gabbiani OPC DA Agent Service - Console Mode
================================================

Initializing OPC Server...
OPC Server started successfully
Server: SCM-Gabbiani OPC DA Server 32bit
Version: 1.0.1

Available Tags:
  - Temperature
  - Pressure
  - Flow
  - Status
  - Alarm

Press Ctrl+C to stop...
```

---

## For System Administrators

### 1. Install

Copy files to installation directory:
```cmd
mkdir "C:\Program Files (x86)\Ga1Agent"
copy Ga1Agent.exe "C:\Program Files (x86)\Ga1Agent\"
xcopy /E config "C:\Program Files (x86)\Ga1Agent\config\"
```

### 2. Configure

Edit `C:\Program Files (x86)\Ga1Agent\config\tags.ini`:
```ini
[Temperature]
Description = Temperature sensor reading
DataType = VT_R4
Address = 0x1000
Scale = 0.1
Offset = 0.0
Units = °C
```

### 3. Install Service
```cmd
cd "C:\Program Files (x86)\Ga1Agent"
Ga1Agent.exe install
```

### 4. Start Service
```cmd
Ga1Agent.exe start
```

Or use Services Manager (services.msc):
1. Find "SCM-Gabbiani OPC DA Agent Service"
2. Right-click > Start

### 5. Verify

Check Event Viewer:
```cmd
eventvwr.msc
# Navigate to Windows Logs > Application
# Look for "GabbianiOPCDA" events
```

Expected events:
- "Service started"
- "OPC Server initialized successfully"

---

## For OPC Client Users

### 1. Connect to Server

**Server Details:**
- Name: `GabbianiOPCDA`
- Node: `localhost` (or server machine name)
- Protocol: OPC DA 2.0/3.0

**Using Kepware:**
1. Launch Kepware OPC Server
2. Add Channel > OPC DA Client
3. Browse for `GabbianiOPCDA`
4. Connect

**Using Matrikon OPC Explorer:**
1. Launch Matrikon OPC Explorer
2. Add Server > Local
3. Select `GabbianiOPCDA`
4. Connect

### 2. Browse Tags

Available default tags:
- `Temperature` - °C (Float)
- `Pressure` - bar (Float)
- `Flow` - l/min (Float)
- `Status` - Status word (Integer)
- `Alarm` - Alarm state (Boolean)

### 3. Monitor Values

Add tags to your client and watch real-time updates.

Update rate: 1000ms (configurable in client)

---

## Common Tasks

### Check Service Status
```cmd
sc query GabbianiOPCDA
```

### Stop Service
```cmd
Ga1Agent.exe stop
# Or
sc stop GabbianiOPCDA
```

### Restart Service
```cmd
Ga1Agent.exe stop
Ga1Agent.exe start
```

### Uninstall Service
```cmd
Ga1Agent.exe stop
Ga1Agent.exe uninstall
```

### View Logs
```cmd
# Event Viewer
eventvwr.msc

# Or command line
wevtutil qe Application /q:"*[System[Provider[@Name='GabbianiOPCDA']]]" /f:text /rd:true /c:20
```

---

## Troubleshooting

### Service Won't Start

**Check COM Port:**
```cmd
# List available COM ports
mode
```

**Check Device Connection:**
- Verify cable is connected
- Check device power
- Verify COM port settings (baud rate, parity, etc.)

### No Data from Device

**Test in Console Mode:**
```cmd
Ga1Agent.exe console
# Check if tags show values or errors
```

**Verify Tag Configuration:**
- Check addresses in tags.ini
- Verify data types match device
- Confirm scaling factors

### OPC Client Can't Connect

**Verify Service is Running:**
```cmd
sc query GabbianiOPCDA
# Should show STATE: RUNNING
```

**Check Firewall:**
```cmd
# Add firewall rule
netsh advfirewall firewall add rule name="OPC DA Server" dir=in action=allow program="C:\Program Files (x86)\Ga1Agent\Ga1Agent.exe" enable=yes
```

**Configure DCOM:**
```cmd
dcomcnfg
# Configure OPC Server DCOM permissions
```

---

## Serial Port Configuration

Default settings:
- Port: COM1
- Baud: 9600
- Data bits: 8
- Parity: None
- Stop bits: 1

To change, modify in Device Manager or update source code in `src/server/opc_server.cpp`:
```cpp
device_.Connect(L"COM1", 9600);  // Change COM port and baud rate
```

---

## Performance Tuning

### Update Rate
In OPC client, set group update rate:
- Faster: 100ms (higher CPU usage)
- Default: 1000ms (recommended)
- Slower: 5000ms (lower CPU usage)

### Tag Count
Tested configurations:
- Small: 1-10 tags
- Medium: 10-100 tags
- Large: 100-1000 tags

---

## Security

### Service Account
Run as dedicated service account:
```cmd
sc config GabbianiOPCDA obj= ".\ServiceAccount" password= "password"
```

### File Permissions
Restrict config file access:
```cmd
icacls "C:\Program Files (x86)\Ga1Agent\config" /inheritance:r /grant "Administrators:(OI)(CI)F" /grant "SYSTEM:(OI)(CI)F"
```

### Network Access
Limit OPC access using Windows Firewall:
```cmd
# Allow only specific IP
netsh advfirewall firewall add rule name="OPC DA Server" dir=in action=allow program="C:\Program Files (x86)\Ga1Agent\Ga1Agent.exe" remoteip=192.168.1.100
```

---

## Getting Help

### Documentation
- `README.md` - Overview and features
- `BUILD.md` - Build instructions
- `DEPLOYMENT.md` - Detailed deployment guide
- `API.md` - API reference for developers
- `SUMMARY.md` - Project summary

### Support Channels
- GitHub Issues: Report bugs or request features
- Documentation: Check relevant .md files
- Event Log: Check Windows Event Viewer for errors

### Useful Commands

**List all Ga1Agent events:**
```cmd
wevtutil qe Application /q:"*[System[Provider[@Name='GabbianiOPCDA']]]" /f:text
```

**Monitor real-time events:**
```cmd
wevtutil qe Application /q:"*[System[Provider[@Name='GabbianiOPCDA']]]" /f:text /rd:false /e:root\cimv2 /select "Win32_NTLogEvent where LogFile='Application' and SourceName='GabbianiOPCDA'"
```

**Export configuration:**
```cmd
copy "C:\Program Files (x86)\Ga1Agent\config\tags.ini" backup_tags.ini
```

---

## Next Steps

After successful installation:

1. ✅ Verify service is running
2. ✅ Test with console mode
3. ✅ Configure your specific tags
4. ✅ Connect OPC client
5. ✅ Monitor data in real-time
6. ✅ Set up monitoring and alerts
7. ✅ Configure automatic startup
8. ✅ Document your setup

---

**Version:** 1.0.0  
**Platform:** Windows 7+ (32-bit compatible)  
**License:** As specified in repository  
**Support:** See GitHub repository
