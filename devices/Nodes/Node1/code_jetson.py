import serial
import time
import os
# Set up the serial connection to the Feather
serial_port = '/dev/ttyACM0' # '/dev/ttyUSB0'  # Adjust this to your serial port
baud_rate = 115200
serial_connection = serial.Serial(serial_port, baud_rate, timeout=1)
time.sleep(2)
def send_message(message, delay_seconds):
    serial_connection.write(message.encode())
    print(f"Sent: {message}")
    time.sleep(delay_seconds)

def main():
    try:
        # Send messages
        send_message("Estado sistema OK\n", 5)
        for x in range(4):
            send_message("12,14,15,16,16,16\n", 5)
        send_message("END RAW DATA\n", 5)

    except Exception as e:
        print(f"Error: {e}")

    finally:
        if serial_connection.is_open:
            serial_connection.close()
        print("Serial connection closed.")
         # Shutdown the Jetson Nano
        print("Shutting down Jetson.")
        #os.system('sudo shutdown now')

if __name__ == "__main__":
    main()
