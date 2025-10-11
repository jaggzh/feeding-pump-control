# ESP Pump Control - API Endpoints

## Base URL
`http://<device-ip>/` (default: configured in wifi_config.h)

---

## Motor Control

### `/fwdon`
**Method:** GET  
**Description:** Enable forward motor hold mode  
**Response:** `Forward Enabled`  
**Example:**
```bash
curl http://192.168.0.10/fwdon
```

### `/revon`
**Method:** GET  
**Description:** Enable reverse motor hold mode  
**Response:** `Reverse Enabled`  
**Example:**
```bash
curl http://192.168.0.10/revon
```

### `/off`
**Method:** GET  
**Description:** Turn off motor (all channels)  
**Response:** `Pump turned OFF`  
**Example:**
```bash
curl http://192.168.0.10/off
```

---

## Safety Lock Control

### `/lock`
**Method:** GET  
**Description:** Enable motor lock (prevents motor operation, safety override)  
**Response:** `Locked`  
**Example:**
```bash
curl http://192.168.0.10/lock
```

### `/unlock`
**Method:** GET  
**Description:** Disable motor lock (allow motor operation)  
**Response:** `Unlocked`  
**Example:**
```bash
curl http://192.168.0.10/unlock
```

---

## Operation Mode Control

### `/mode_pump_only`
**Method:** GET  
**Description:** Set operation mode to PUMP_ONLY (motor works, TCP signals disabled)  
**Response:** `Mode set to PUMP_ONLY`  
**Example:**
```bash
curl http://192.168.0.10/mode_pump_only
```

### `/mode_signal_only`
**Method:** GET  
**Description:** Set operation mode to SIGNAL_ONLY (motor disabled, TCP signals work)  
**Note:** Automatically turns off motor when activated  
**Response:** `Mode set to SIGNAL_ONLY`  
**Example:**
```bash
curl http://192.168.0.10/mode_signal_only
```

### `/mode_both`
**Method:** GET  
**Description:** Set operation mode to BOTH (motor and TCP signals both work - default)  
**Response:** `Mode set to BOTH`  
**Example:**
```bash
curl http://192.168.0.10/mode_both
```

### `/mode_status`
**Method:** GET  
**Description:** Query current operation mode  
**Response:** `Current mode: <MODE>`  
**Example:**
```bash
curl http://192.168.0.10/mode_status
```

---

## Alarm/Signal Configuration

### `/set_alarm_host`
**Method:** GET  
**Description:** Set runtime alarm destination host and ports  
**Parameters:**
- `host` (required) - IP address of alarm server
- `port_hid` (required) - Port for HID/general events
- `port_hold` (optional) - Port for hold events (defaults to port_hid)
- `port_toolong` (optional) - Port for too-long safety events (defaults to port_hid)

**Response:** `Alarm host set to: <host> ports: HID=<port> HOLD=<port> TOOLONG=<port>`  
**Examples:**
```bash
# Set all ports explicitly
curl "http://192.168.0.10/set_alarm_host?host=192.168.1.50&port_hid=10&port_hold=7&port_toolong=8"

# Set host and one port (others default to port_hid)
curl "http://192.168.0.10/set_alarm_host?host=192.168.1.50&port_hid=10"
```

### `/reset_alarm_host`
**Method:** GET  
**Description:** Reset alarm configuration to hardcoded defaults from wifi_config.h  
**Response:** `Alarm host reset to defaults`  
**Example:**
```bash
curl http://192.168.0.10/reset_alarm_host
```

---

## Status & Monitoring

### `/status`
**Method:** GET  
**Description:** Get current system status  
**Response:** `<ms> ms Motor:<state> [LOCKED/Unlocked] Mode:<mode> PotRate:<value> PotX:<value>`  
**Example:**
```bash
curl http://192.168.0.10/status
```

### `/`
**Method:** GET  
**Description:** Web interface dashboard  
**Response:** HTML page with controls and live status log  
**Features:**
- Real-time status updates
- Clickable control buttons
- Operation mode display
- Alarm configuration display
- Scrolling event log

---

## Debug Control

### `/debug_inc`
**Method:** GET  
**Description:** Increase debug verbosity level  
**Response:** `Increased debug`  
**Example:**
```bash
curl http://192.168.0.10/debug_inc
```

### `/debug_dec`
**Method:** GET  
**Description:** Decrease debug verbosity level  
**Response:** `Decreased debug`  
**Example:**
```bash
curl http://192.168.0.10/debug_dec
```

---

## Default Configuration

**Hardcoded Defaults** (from wifi_config.h):
- Alarm Host: `192.168.0.20`
- HID Port: `10`
- Hold Port: `7`
- Too-Long Port: `8`
- Operation Mode: `MODE_BOTH`
- Motor Lock: `false` (unlocked)

**Note:** Runtime configuration changes (alarm host/ports, operation mode) are lost on device reset and revert to defaults.

---

## TCP Signal Events

When operation mode allows TCP signals (MODE_SIGNAL_ONLY or MODE_BOTH), the following events are sent to the configured alarm host:

- `pat-press` - Patient button pressed
- `pat-hold` - Patient button held
- `pat-safety` - Patient button safety shutoff triggered
- `pat-release` - Patient button released
- `pat-release-from-press` - Released from press state
- `pat-release-from-hold-start` - Released from hold start
- `pat-release-from-safety` - Released from safety mode
- `pat-rev-hold--cancel-by-pat-press` - Reverse hold cancelled by patient press
- `potx` - Potentiometer X value changed
- General alarm triggers on specific port

---

## Usage Examples

### Complete Workflow Example
```bash
# Check current status
curl http://192.168.0.10/status

# Set to signal-only mode for testing
curl http://192.168.0.10/mode_signal_only

# Configure custom alarm destination
curl "http://192.168.0.10/set_alarm_host?host=192.168.1.100&port_hid=5000"

# Return to normal operation
curl http://192.168.0.10/mode_both

# Enable safety lock
curl http://192.168.0.10/lock

# Reset everything to defaults
curl http://192.168.0.10/unlock
curl http://192.168.0.10/reset_alarm_host
curl http://192.168.0.10/mode_both
```