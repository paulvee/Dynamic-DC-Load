# Test Plan - Dynamic DC Load Firmware v7.0.3

Comprehensive testing plan for firmware changes from v7.0.0 to v7.0.3

**Test Date:** _______________  
**Tester:** _______________  
**Firmware Version:** 7.0.3  
**Python App Version:** _______________

---

## Pre-Test Setup

### Hardware Requirements
- [ ] ESP32 Dynamic Load hardware connected
- [ ] Power supply available
- [ ] Test battery (low capacity recommended, e.g., 200-500mAh)
- [ ] USB cable connected to PC
- [ ] All safety connections verified

### Software Requirements
- [ ] Firmware v7.0.3 uploaded to ESP32
- [ ] Python Battery Tester app v2.0.h or later installed
- [ ] Serial port identified (e.g., COM3)
- [ ] Serial monitor available (Arduino IDE, PlatformIO, or Python app)

### Initial Verification
- [ ] Power on device - OLED displays correctly
- [ ] Rotary encoder responds to rotation and button press
- [ ] Fan operation normal
- [ ] No error messages on startup

---

## Test Suite 1: Basic Functionality (Baseline)

**Purpose:** Verify core functionality still works after migration to PlatformIO

### Test 1.1: Manual Mode Selection
**Steps:**
1. Power on device
2. Use rotary encoder to navigate menus
3. Select each mode: CC, CV, CP, CR, Battery Test

**Expected Results:**
- [ ] All modes accessible via rotary encoder
- [ ] OLED displays correct mode name
- [ ] No crashes or freezes
- [ ] Menu navigation is smooth

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 1.2: Constant Current (CC) Mode
**Steps:**
1. Select CC mode
2. Set current to 0.5A
3. Connect test load (or battery)
4. Monitor operation for 30 seconds

**Expected Results:**
- [ ] Current maintains at 0.5A ±0.01A
- [ ] Voltage displayed accurately
- [ ] Power calculated correctly (V × I)
- [ ] OLED updates regularly

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 1.3: Safety Protections
**Steps:**
1. Test OVP: Apply voltage above configured limit
2. Test OCP: Set current beyond safe limit
3. Test OTP: (if accessible) trigger temperature alarm

**Expected Results:**
- [ ] OVP triggers at correct voltage threshold
- [ ] OCP triggers at correct current threshold
- [ ] Device shuts down load safely
- [ ] Error message displayed on OLED

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

## Test Suite 2: Recovery Monitoring (v7.0.1)

**Purpose:** Verify new recovery timeout feature with configurable 1-30 minute timeout

### Test 2.1: Recovery Countdown Display
**Steps:**
1. Start battery test via Python app
2. Set recovery_time to 2 minutes (parameter 9)
3. Wait for battery to reach cutoff voltage
4. Observe recovery countdown

**Expected Results:**
- [ ] Test stops at cutoff voltage
- [ ] Recovery countdown starts immediately
- [ ] OLED displays "Recovery: XX:XX" format
- [ ] Countdown decrements every second
- [ ] Current set to 0A during recovery

**Status:** ⬜ PASS  ⬜ FAIL  
**Recovery Time Set:** _____ minutes  
**Actual Countdown Duration:** _____ minutes  
**Notes:** _______________________________________________

---

### Test 2.2: Recovery Voltage Monitoring
**Steps:**
1. During recovery period from Test 2.1
2. Monitor battery voltage recovery
3. Note voltage at start and end of recovery

**Expected Results:**
- [ ] Voltage displayed continuously during recovery
- [ ] Voltage increases as battery recovers
- [ ] No load current applied during recovery
- [ ] Test completes after recovery timeout

**Voltage at Recovery Start:** _____ V  
**Voltage at Recovery End:** _____ V  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 2.3: Recovery Timeout Variations
**Steps:**
1. Test with recovery_time = 1 minute
2. Test with recovery_time = 5 minutes
3. Test with recovery_time = 15 minutes

**Expected Results:**
- [ ] 1-minute test completes in ~1 minute
- [ ] 5-minute test completes in ~5 minutes
- [ ] 15-minute test completes in ~15 minutes
- [ ] Timing accuracy within ±5 seconds

**1-Min Actual:** _______________  
**5-Min Actual:** _______________  
**15-Min Actual:** _______________  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

## Test Suite 3: Filtered Cutoff Detection (v7.0.1)

**Purpose:** Verify that cutoff detection uses 16-sample moving average to prevent false triggers

### Test 3.1: Stable Cutoff Detection
**Steps:**
1. Start battery test with cutoff voltage = 3.0V
2. Use a battery that naturally reaches 3.0V
3. Observe cutoff behavior

**Expected Results:**
- [ ] Test stops when voltage reaches 3.0V
- [ ] No premature cutoff from voltage spikes
- [ ] Cutoff is smooth and consistent
- [ ] Recovery phase starts immediately after

**Cutoff Voltage Setting:** _____ V  
**Actual Cutoff Voltage:** _____ V  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 3.2: Noise Rejection (if possible)
**Steps:**
1. Start battery test
2. Introduce brief voltage spikes (disconnect/reconnect battery momentarily)
3. Observe if test continues or stops prematurely

**Expected Results:**
- [ ] Brief spikes do not trigger cutoff
- [ ] Filtered voltage smooths out noise
- [ ] Test continues until true cutoff voltage reached
- [ ] No false positives from transient spikes

**Status:** ⬜ PASS  ⬜ FAIL  ⬜ N/A  
**Notes:** _______________________________________________

---

## Test Suite 4: Auto Mode Switch (v7.0.2)

**Purpose:** Verify automatic switching to Battery Test mode when Python app sends AUTO_BT command

### Test 4.1: Manual Mode - Verify No Auto-Switch
**Steps:**
1. Power on ESP32
2. Manually navigate to CC mode (not Battery Test mode)
3. Do NOT start Python app
4. Wait 30 seconds

**Expected Results:**
- [ ] Device stays in CC mode
- [ ] No automatic mode change
- [ ] OLED shows CC mode
- [ ] Normal operation

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 4.2: Auto Mode Switch from Idle
**Steps:**
1. Power on ESP32 (leave in main menu or any mode)
2. Open Python Battery Tester app
3. Configure test parameters
4. Click "Start Test" button
5. Observe ESP32 OLED display

**Expected Results:**
- [ ] Python app sends AUTO_BT command
- [ ] ESP32 automatically switches to Battery Test mode
- [ ] ESP32 responds with ACK_BT
- [ ] Python app receives acknowledgment
- [ ] Test starts without manual mode selection
- [ ] OLED shows "Battery Test" mode

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 4.3: Auto Mode Switch from Different Mode
**Steps:**
1. Manually select CV mode on ESP32
2. Leave device in CV mode (not running a test)
3. Start test from Python app

**Expected Results:**
- [ ] Device automatically switches from CV to Battery Test mode
- [ ] Test starts successfully
- [ ] No manual intervention required

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 4.4: Firmware Upload Compatibility
**Steps:**
1. Close Python app completely
2. Use PlatformIO or Arduino IDE to upload firmware
3. Observe upload process

**Expected Results:**
- [ ] Firmware uploads successfully
- [ ] No interference from AUTO_BT command handler
- [ ] esptool completes normally
- [ ] Device boots correctly after upload

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

## Test Suite 5: Communication Watchdog (v7.0.3)

**Purpose:** Verify 60-second timeout aborts test if PC disconnects or crashes

### Test 5.1: Normal Operation with Heartbeat
**Steps:**
1. Start battery test from Python app
2. Let test run for 5 minutes
3. Observe normal operation
4. Monitor serial communication (optional: use serial monitor)

**Expected Results:**
- [ ] Test runs normally
- [ ] Python app sends HB (heartbeat) every 30 seconds
- [ ] ESP32 receives heartbeats (updates lastSerialActivity)
- [ ] No timeout errors
- [ ] Test completes or continues normally

**Test Duration:** _____ minutes  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 5.2: Communication Timeout - Cable Disconnect
**Steps:**
1. Start battery test from Python app
2. Wait 30 seconds for test to stabilize
3. **Carefully disconnect USB cable** (safety: use low current/low capacity battery)
4. Wait 60-70 seconds
5. Observe ESP32 OLED display

**Expected Results:**
- [ ] Test runs normally before disconnect
- [ ] After disconnect, no heartbeat received
- [ ] After ~60 seconds, watchdog triggers
- [ ] Test aborts automatically
- [ ] OLED displays: "Comm Timeout - PC Disconnected"
- [ ] Load current set to 0A (safe state)
- [ ] No runaway discharge

**Time to Timeout:** _____ seconds  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 5.3: Communication Timeout - App Crash Simulation
**Steps:**
1. Start battery test from Python app
2. Wait 30 seconds
3. **Force close Python app** (Task Manager or Alt+F4)
4. Do NOT disconnect USB cable
5. Wait 60-70 seconds
6. Observe ESP32 behavior

**Expected Results:**
- [ ] Python app closes/crashes
- [ ] Heartbeat stops being sent
- [ ] After ~60 seconds, ESP32 watchdog triggers
- [ ] Test aborts with timeout message
- [ ] Device enters safe state (0A load)

**Time to Timeout:** _____ seconds  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 5.4: Watchdog Reset on Activity
**Steps:**
1. Start battery test
2. Wait 45 seconds (close to timeout)
3. Send manual serial command from Python app or serial monitor (e.g., parameters update)
4. Wait another 45 seconds
5. Send another command
6. Repeat for 3 minutes total

**Expected Results:**
- [ ] Each command resets watchdog timer
- [ ] No timeout occurs despite elapsed time
- [ ] Test continues normally
- [ ] lastSerialActivity updates on each command

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 5.5: Manual Cancel During Watchdog Period
**Steps:**
1. Start battery test
2. Wait 30 seconds
3. Click "Cancel" button in Python app (or send 999 command)
4. Verify test stops immediately

**Expected Results:**
- [ ] Cancel command received
- [ ] Test stops immediately (within 1 second)
- [ ] No timeout message displayed
- [ ] Normal cancel behavior
- [ ] Load set to 0A

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 5.6: Heartbeat Transparency
**Steps:**
1. Monitor serial communication during test (use serial monitor)
2. Observe HB commands being sent every 30 seconds
3. Verify no user-visible effects

**Expected Results:**
- [ ] HB commands visible in serial monitor
- [ ] HB commands sent approximately every 30 seconds
- [ ] No messages displayed to user about heartbeat
- [ ] No disruption to test operation
- [ ] Completely transparent operation

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

## Test Suite 6: Integration Testing

**Purpose:** Verify all features work together correctly

### Test 6.1: Complete Battery Test with All Features
**Steps:**
1. Start Python Battery Tester app
2. Configure test:
   - Discharge current: 0.5A
   - Cutoff voltage: 3.0V
   - Recovery time: 2 minutes
3. Click "Start Test"
4. Let test run to completion

**Expected Results:**
- [ ] Auto mode switch activates (AUTO_BT → ACK_BT)
- [ ] Test starts automatically
- [ ] Heartbeat maintains connection
- [ ] Voltage filtering prevents false cutoffs
- [ ] Test reaches cutoff voltage smoothly
- [ ] Recovery countdown starts (2 minutes)
- [ ] Recovery completes
- [ ] Final voltage recorded
- [ ] Test completion message shown
- [ ] Data saved/exported correctly

**Test Duration:** _______________  
**Final Capacity:** _______________  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 6.2: Rapid Test Start/Stop Cycles
**Steps:**
1. Start test from Python app
2. After 10 seconds, cancel test
3. Immediately start new test
4. Repeat 5 times

**Expected Results:**
- [ ] Each test starts cleanly
- [ ] Cancel works reliably
- [ ] No crashes or hangs
- [ ] Mode switching works repeatedly
- [ ] No memory leaks or performance degradation

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 6.3: Long-Duration Test with Watchdog
**Steps:**
1. Start test with higher capacity battery (1000+ mAh)
2. Let test run for 2+ hours
3. Monitor heartbeat operation throughout

**Expected Results:**
- [ ] Test runs continuously without timeout
- [ ] Heartbeat consistently sent every 30 seconds
- [ ] No false timeout errors
- [ ] Memory usage stable
- [ ] Performance consistent

**Test Duration:** _______________  
**Battery Capacity:** _______________  
**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

## Test Suite 7: Backward Compatibility

**Purpose:** Verify compatibility with older Python app versions

### Test 7.1: Legacy Python App (without AUTO_BT)
**Setup:** Use Python app version < 2.0.h (if available, or manually disable AUTO_BT)

**Steps:**
1. Manually select Battery Test mode on ESP32
2. Send test parameters from old Python app
3. Run complete test

**Expected Results:**
- [ ] Manual mode selection still works
- [ ] Old Python app works without AUTO_BT command
- [ ] Test executes normally
- [ ] Full backward compatibility maintained

**Status:** ⬜ PASS  ⬜ FAIL  ⬜ N/A (no old version available)  
**Notes:** _______________________________________________

---

### Test 7.2: Legacy Python App (without Heartbeat)
**Setup:** Use Python app without heartbeat feature

**Steps:**
1. Start test with app that doesn't send HB commands
2. Test should timeout after 60 seconds

**Expected Results:**
- [ ] Test starts normally
- [ ] After 60 seconds, watchdog triggers timeout
- [ ] Test aborts safely
- [ ] User knows they need to upgrade Python app

**Status:** ⬜ PASS  ⬜ FAIL  ⬜ N/A  
**Notes:** _______________________________________________

---

## Test Suite 8: Edge Cases and Error Conditions

### Test 8.1: Battery Removal During Test
**Steps:**
1. Start test
2. Let run 30 seconds
3. Disconnect battery (but keep USB connected)
4. Observe behavior

**Expected Results:**
- [ ] Voltage drops to ~0V
- [ ] Device handles gracefully (no crash)
- [ ] May trigger cutoff or error condition
- [ ] Safe recovery possible

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 8.2: Parameter Update During Test
**Steps:**
1. Start test
2. While running, send new test parameters from Python app
3. Observe behavior

**Expected Results:**
- [ ] Either: new parameters applied mid-test, OR
- [ ] Parameters rejected until current test completes
- [ ] No crash or undefined behavior
- [ ] Watchdog resets on parameter reception

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 8.3: Multiple Cancel Commands
**Steps:**
1. Start test
2. Send cancel command (999)
3. Immediately send 2-3 more cancel commands
4. Verify clean shutdown

**Expected Results:**
- [ ] First cancel stops test
- [ ] Subsequent cancels ignored or handled gracefully
- [ ] No crash or errors
- [ ] Device returns to ready state

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 8.4: Zero Recovery Time
**Steps:**
1. Set recovery_time = 0 minutes
2. Run battery test to cutoff

**Expected Results:**
- [ ] Test stops at cutoff
- [ ] Either: skips recovery entirely, OR
- [ ] Shows 0:00 recovery and completes immediately
- [ ] No crash or hang

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 8.5: Maximum Recovery Time
**Steps:**
1. Set recovery_time = 30 minutes
2. Run test to cutoff (or manually trigger recovery if possible)
3. Brief observation (don't wait full 30 min unless necessary)

**Expected Results:**
- [ ] Recovery countdown shows 30:00
- [ ] Countdown begins properly
- [ ] Display shows minutes:seconds correctly
- [ ] No overflow or display errors

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

## Test Suite 9: Performance and Stability

### Test 9.1: RTOS Task Performance
**Steps:**
1. Run any test
2. Observe OLED update rate
3. Check response to rotary encoder inputs
4. Monitor serial response time

**Expected Results:**
- [ ] OLED updates smoothly (no flickering)
- [ ] Rotary encoder responds instantly
- [ ] Serial commands processed quickly (<100ms)
- [ ] No task starvation or priority issues

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

### Test 9.2: Memory Stability
**Steps:**
1. Run multiple tests back-to-back
2. Monitor for any degradation
3. Check for heap errors (if debugging enabled)

**Expected Results:**
- [ ] Performance consistent across multiple tests
- [ ] No memory leaks
- [ ] No heap corruption
- [ ] Device can run indefinitely

**Status:** ⬜ PASS  ⬜ FAIL  
**Notes:** _______________________________________________

---

## Test Summary

| Test Suite | Tests | Pass | Fail | N/A |
|-----------|-------|------|------|-----|
| 1. Basic Functionality | 3 | ___ | ___ | ___ |
| 2. Recovery Monitoring | 3 | ___ | ___ | ___ |
| 3. Filtered Cutoff | 2 | ___ | ___ | ___ |
| 4. Auto Mode Switch | 4 | ___ | ___ | ___ |
| 5. Comm Watchdog | 6 | ___ | ___ | ___ |
| 6. Integration | 3 | ___ | ___ | ___ |
| 7. Backward Compat | 2 | ___ | ___ | ___ |
| 8. Edge Cases | 5 | ___ | ___ | ___ |
| 9. Performance | 2 | ___ | ___ | ___ |
| **TOTAL** | **30** | ___ | ___ | ___ |

---

## Critical Issues Found

| Test ID | Issue Description | Severity | Status |
|---------|------------------|----------|---------|
| | | | |
| | | | |
| | | | |

**Severity Levels:** Critical / High / Medium / Low

---

## Recommendations

### Must Fix Before Release
- [ ] All Critical severity issues resolved
- [ ] Watchdog functionality verified (Test 5.2, 5.3)
- [ ] Auto mode switch working (Test 4.2)
- [ ] No crashes or data corruption

### Should Fix
- [ ] All High severity issues addressed
- [ ] Edge cases handled gracefully

### Nice to Have
- [ ] Medium/Low severity improvements
- [ ] Performance optimizations

---

## Sign-Off

**Tested By:** _______________  
**Date:** _______________  
**Overall Status:** ⬜ APPROVED FOR RELEASE  ⬜ NEEDS REWORK  

**Comments:**
________________________________________________________________
________________________________________________________________
________________________________________________________________

---

*Test Plan Version: 1.0*  
*Created: March 11, 2026*  
*For Firmware: v7.0.3*
