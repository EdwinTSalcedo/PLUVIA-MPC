import numpy as np
import time
import serial
import os
from datetime import datetime

serial_connection = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

# Crear la carpeta "data" si no existe
if not os.path.exists('data'):
    os.makedirs('data')

# Obtener la fecha y hora actuales para el nombre del archivo
current_time = datetime.now().strftime("%Y-%m-%d_%H-%M")
file_name = f"data/{current_time}.txt"

# Threshold inicial
initial_threshold = 41

# Datos de previsiones de precipitaciones para hoy (24 horas)
# precipitation_forecast_this_week = np.random.rand(7) * 100
precipitation_forecast_this_week =  [10,20,10,10,15,13,35]

# Datos de precipitaciones de la semana anterior (7 días)
# precipitation_last_week = np.random.rand(7) * 100 
precipitation_last_week = [1,2,3,4,5,6,7]

# Promedio de precipitaciones de la semana anterior
average_precipitation_last_week = np.mean(precipitation_last_week)

# Ajustar el threshold inicial si el promedio de la semana pasada es superior al umbral
if average_precipitation_last_week > initial_threshold:
    adjusted_threshold = initial_threshold * 0.95
else:
    adjusted_threshold = initial_threshold

node_status = "OFF"

def determine_node_activation(precipitation, threshold):
    if precipitation < threshold:
        return "ON", [5, 5, 5]  # Encender por 5 minutos
    elif precipitation == threshold:
        return "ON", [10, 10, 10]  # Encender por 10 minutos cada 3 horas
    elif threshold < precipitation <= 90:
        return "ON", [10, 10, 10]  # Encender por 10 minutos cada 2 horas
    elif precipitation > 90:
        return "ON", [10, 10, 10]  # Encender por 10 minutos cada hora
    else:
        return "OFF", [0, 0, 0]

def send_serial_signal(activation_times):
    #command = f"ACTIVATE {activation_times[0]} {activation_times[1]} {activation_times[2]}\n"
    command = f"ACTIVATE 1 {1} {1} {activation_times[2]}\n"
    serial_connection.write(command.encode())
    
with open(file_name, 'w') as file:
    hour = "00:00"
    precipitation = 10
    node_status, activation_times = determine_node_activation(precipitation, adjusted_threshold)
    print(f"Hora {hour}: Precipitación prevista = {precipitation:.2f} mm, Estado del nodo = {node_status}, Tiempos de activación = {activation_times} minutos")
    confirmation = input(f"Confirmar envío de señal de encendido por {activation_times} minutos a la Feather 32u4 (y/n): ").strip().lower()
    if confirmation == 'y':
        send_serial_signal(activation_times)
    response = serial_connection.readline().decode().strip()
    while (response != "End of raw data received." and response != "N3: END RAW DATA"):
        print(f"Respuesta del nodo: {response}")
        response = serial_connection.readline().decode().strip()
        file.write(f"{datetime.now().strftime('%Y-%m-%d %H:%M:%S')} - {response}\n")
    print(f"Respuesta final del nodo: {response}")
    file.write(f"{datetime.now().strftime('%Y-%m-%d %H:%M:%S')} - {response}\n")
serial_connection.close()
print("Programa finalizado y conexión serial cerrada.")
