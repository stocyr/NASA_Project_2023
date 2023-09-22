import serial.tools.list_ports

def get_serial_port_info():
    # List available serial ports
    serial_ports = serial.tools.list_ports.comports()

    for port in serial_ports:
        print(f"Port: {port.device}")
        
        # Check if the port has a USB connection
        if port.hwid:
            # Parse the hardware ID to get product and vendor ID
            hardware_id_parts = port.hwid.split(':')
            if len(hardware_id_parts) >= 3:
                vendor_id = hardware_id_parts[1]
                product_id = hardware_id_parts[2]
                print(f"Vendor ID: {vendor_id}")
                print(f"Product ID: {product_id}")
            else:
                print("Vendor ID and Product ID not found in hardware ID.")
        else:
            print("No USB information available for this port.")
        print("-" * 40)

if __name__ == "__main__":
    get_serial_port_info()