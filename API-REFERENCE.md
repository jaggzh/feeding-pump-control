# ESP Pump Control - API Quick Reference

Base URL: `http://<device-ip>/`

## Motor Control
- `/fwdon` - Enable forward | `/revon` - Enable reverse | `/off` - Turn off motor
- `/lock` - Enable safety lock | `/unlock` - Disable lock

## Function Control (Presets)
- `/hidmode?1` - HID only (auto-disables pump) | `/hidmode?0` - Disable HID
- `/pumpmode?1` - Pump+Alarms only | `/pumpmode?0` - Disable pump
- `/pumphidmode?1` - All enabled (default on boot)

## Function Control (Fine-Grained)
- `/func_pump?0|1` - Toggle pump functionality
- `/func_net_alarms?0|1` - Toggle alarm signals (hold/toolong events)
- `/func_net_hid?0|1` - Toggle HID signals (pat-press, potx, etc)

## Host Configuration
**Alarm Host** (hold/toolong events):
- `/sethost_alarm?ip=IP&port_hold=N&port_toolong=N` - Set alarm host (all params optional, preserves unspecified)
- `/resethost_alarm` - Reset to defaults

**HID Host** (general events):
- `/sethost_hid?ip=IP&port=N` - Set HID host (all params optional, preserves unspecified)
- `/resethost_hid` - Reset to defaults

**Both:**
- `/resethosts_all` - Reset all hosts to defaults

## Status & Debug
- `/status` - Current system status
- `/funcstatus` - Detailed function flags breakdown
- `/` - Web dashboard with live updates
- `/debug_inc` | `/debug_dec` - Adjust debug verbosity

## Defaults (wifi_config.h)
```
HOST_ALARM=192.168.0.20, PORT_ALARM_HOLD=7, PORT_ALARM_TOOLONG=8
HOST_HID=192.168.0.20, PORT_HID=10
Function Flags: FEATSET_PUMPHID (0x07) - all enabled
```

## Function Flags
- `FUNC_PUMP` (0x01) - Motor control
- `FUNC_NET_ALARMS` (0x02) - Alarm signals (hold, toolong)
- `FUNC_NET_HID` (0x04) - HID signals (pat-*, potx)

## Quick Examples
```bash
# Boot state check
curl http://192.168.0.10/funcstatus

# Switch to HID-only mode
curl "http://192.168.0.10/hidmode?1"

# Configure separate hosts
curl "http://192.168.0.10/sethost_alarm?ip=192.168.0.20&port_hold=7&port_toolong=8"
curl "http://192.168.0.10/sethost_hid?ip=192.168.0.25&port=10"

# Just change alarm host IP, keep ports
curl "http://192.168.0.10/sethost_alarm?ip=192.168.1.50"

# Testing: disable pump, keep signals
curl "http://192.168.0.10/func_pump?0"

# Return to full functionality
curl "http://192.168.0.10/pumphidmode?1"
```

## HID Events Sent (when FUNC_NET_HID enabled)
`pat-press`, `pat-hold`, `pat-safety`, `pat-release`, `pat-release-from-press`, `pat-release-from-hold-start`, `pat-release-from-safety`, `pat-rev-hold--cancel-by-pat-press`, `potx`

## Alarm Events Sent (when FUNC_NET_ALARMS enabled)
Hold mode trigger ? PORT_ALARM_HOLD | Too-long safety ? PORT_ALARM_TOOLONG