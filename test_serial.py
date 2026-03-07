"""
Simple test script to verify serial communication with Arduino/ESP32 controller.

This script demonstrates basic connection and protocol usage without the GUI.
"""

import sys
import time
from hardware import SerialController, ArduinoProtocol


def test_connection():
    """Test serial connection and list available ports"""
    print("Battery Tester - Serial Communication Test")
    print("=" * 50)
    
    # List available ports
    ports = SerialController.get_available_ports()
    print(f"\nAvailable COM ports: {ports}")
    
    if not ports:
        print("No COM ports found!")
        return
    
    # Let user select port
    print("\nSelect port number:")
    for i, port in enumerate(ports):
        info = SerialController.get_port_info(port)
        print(f"  {i}: {info}")
    
    try:
        selection = int(input("\nEnter port number: "))
        selected_port = ports[selection]
    except (ValueError, IndexError):
        print("Invalid selection!")
        return
    
    # Connect to port
    print(f"\nConnecting to {selected_port}...")
    controller = SerialController(selected_port, 9600)
    
    if not controller.connect():
        print(f"Failed to connect: {controller.last_error}")
        return
    
    print(f"Connected successfully!")
    print(f"Connection: {controller.get_connection_string()}")
    
    # Create protocol handler
    protocol = ArduinoProtocol(controller.serial)
    
    # Ask if user wants to reset controller
    reset = input("\nReset controller? (y/n): ").lower() == 'y'
    if reset:
        print("Resetting controller...")
        protocol.reset_controller()
        print("Reset complete. Waiting for controller to boot...")
        time.sleep(2)
    
    # Clear any startup messages
    protocol.clear_buffer()
    
    # Test parameters
    print("\n" + "=" * 50)
    print("Test Configuration:")
    print("=" * 50)
    current_ma = 50
    cutoff_v = 3.0
    capacity_mah = 100
    sample_sec = 5
    
    print(f"Discharge Current: {current_ma} mA")
    print(f"Cutoff Voltage: {cutoff_v} V")
    print(f"Capacity: {capacity_mah} mAh")
    print(f"Sample Interval: {sample_sec} seconds")
    
    # Calculate max time
    max_time = int((capacity_mah / current_ma) * 60 + 30)
    print(f"Max Test Time: {max_time} minutes")
    
    input("\nPress Enter to send test parameters...")
    
    # Send test parameters
    print("\nSending test parameters to controller...")
    success = protocol.send_test_parameters(
        current_ma=current_ma,
        cutoff_voltage=cutoff_v,
        time_limit_minutes=max_time,
        sample_interval_sec=sample_sec,
        kp=50,
        beep_enabled=False
    )
    
    if not success:
        print("Failed to send parameters!")
        controller.disconnect()
        return
    
    print("Parameters sent successfully!")
    print("\nWaiting for data from controller...")
    print("Press Ctrl+C to stop\n")
    
    # Read and display data
    try:
        packet_count = 0
        while True:
            packets = protocol.read_data()
            
            for packet_type, value in packets:
                packet_count += 1
                
                if packet_type == 'reading':
                    # Combined packet with all data
                    reading = value
                    print(f"\n[Packet #{packet_count}]")
                    print(f"  Time: {reading.elapsed_time}")
                    print(f"  Voltage: {reading.voltage:.3f} V")
                    print(f"  Current: {reading.current_ma:.1f} mA")
                    print(f"  Capacity: {reading.capacity_mah:.2f} mAh")
                    
                elif packet_type == 'message':
                    print(f"\n>>> MESSAGE: {value}")
                    if 'Finished' in value or 'Cancelled' in value or 'Exceeded' in value:
                        print("\nTest completed or terminated.")
                        break
                
                elif packet_type == 'voltage':
                    print(f"  Voltage: {value:.3f} V")
                elif packet_type == 'current':
                    print(f"  Current: {value:.1f} mA")
                elif packet_type == 'capacity':
                    print(f"  Capacity: {value:.2f} mAh")
                elif packet_type == 'time':
                    print(f"  Time: {value}")
            
            time.sleep(0.1)  # Small delay to avoid spinning
            
    except KeyboardInterrupt:
        print("\n\nStopping test...")
        protocol.cancel_test()
        time.sleep(1)
    
    finally:
        controller.disconnect()
        print("Disconnected.")


def main():
    try:
        test_connection()
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()


if __name__ == '__main__':
    main()
