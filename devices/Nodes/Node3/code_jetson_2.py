import time
import os
from smbus2 import SMBus

# Configuración del bus I2C
bus_number = 1  # Ajustar según el bus I2C utilizado en la Jetson Nano
i2c_address = 0x04  # La dirección I2C de la Feather
bus = SMBus(bus_number)

def send_i2c_message(message, delay_seconds):
    bytes_to_send = message.encode('utf-8')
    bus.write_i2c_block_data(i2c_address, 0, bytes_to_send)
    print(f"Sent: {message}")
    time.sleep(delay_seconds)

def main():
    try:
        # Enviar mensajes
        send_i2c_message("Estado sistema OK", 5)
        for x in range(4):
            send_i2c_message("12,14,15,16,16,16", 5)
        send_i2c_message("END RAW DATA", 5)

    except Exception as e:
        print(f"Error: {e}")

    finally:
        bus.close()
        print("I2C connection closed.")
        # Apagar la Jetson Nano
        print("Shutting down Jetson.")
        #os.system('sudo shutdown now')

if __name__ == "__main__":
    main()
