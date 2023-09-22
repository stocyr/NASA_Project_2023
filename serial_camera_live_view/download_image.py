import serial.tools.list_ports
import sys

# Define the desired vendor and product ID
desired_vendor_id = 0x239A
desired_product_id = 0x802B

# Enumerate available serial ports
available_ports = list(serial.tools.list_ports.comports())

# Initialize variables to hold the serial port and match status
serial_port_name = None
port_matched = False

# Iterate through the available ports
for port_info in available_ports:
    # Check if the port's VID (Vendor ID) and PID (Product ID) match the desired values
    if port_info.vid == desired_vendor_id and port_info.pid == desired_product_id:
        if port_matched:
            print(f'Found second idential hardware on {port_info.device}, only connecting to the first one')
            break
        else:
            print(f'Found hardware on {port_info.device}.')
            serial_port_name = port_info.device
            port_matched = True

# Check if a matching port was found
if port_matched:
    try:
        # Open the matching serial port
        serial_port = serial.Serial(serial_port_name, baudrate=115200, timeout=1)
        print(f"Connected to {serial_port_name}.")
        
        # Now you can read from and write to 'ser' as needed
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        sys.exit(-1)
else:
    print("No matching serial port found.")
    sys.exit(-1)


try:
    serial_port.reset_input_buffer()

    # Send "T" followed by "view" and a carriage return
    input('Press ENTER to send "T"')
    serial_port.write(b'T\r')

    while True:
        input('Press ENTER to send "view"')
        serial_port.reset_input_buffer()
        serial_port.write(b'view\r')

        # Read the data until the photo data arrives
        data_line_raw = []
        image_data_line = []
        line_counter = 0
        while True:
            data_raw = serial_port.read_until(expected=b'\r', size=None)
            if data_raw == b'\n=>' and len(image_data_line):
                # Problem: last "=>" doesn't end with a newline --> have to catch it explicitely here
                # After image data: can convert now
                print('(finished downloading image)')
                image_data = ''.join(image_data_line)
                print(f'Image data complete, {len(image_data) // 2} bytes received.')
                # Convert the received data to binary and write it to a file
                with open('test.jpg', 'wb') as file:
                    file.write(bytes.fromhex(image_data))
                break

            data_line_raw.append(data_raw)
            if not data_raw.endswith(b'\r') and not data_raw.endswith(b'\n'):
                # Continue getting bytes
                continue
            else:
                # Full line of data available
                data_line_raw = b''.join(data_line_raw)

                try:
                    # Try to decode byte array
                    data = data_line_raw.decode('utf-8', errors='backslashreplace')
                except Exception as e:
                    print(f'Could not decode something in {data_line_raw}\n{e}')
                    data = ''
                    raise ValueError()
                data_line_raw = []
                
                # Remove starting or trailing white spaces (due to inconsistent use of '\r\n' and '\n\r')
                data = data.strip()

                print(f'Line {line_counter:02d}: {data}')
                line_counter += 1
                
                try:
                    # Check if data line is hex-only -- throws exception if now
                    int(data, 16)
                    image_data_line.append(data)
                except:
                    # Before image data --> continue reading lines
                    continue
    # Close the serial port
finally:
    serial_port.close()
