import os
import paho.mqtt.client as mqtt
import time
import serial
import json
import threading
import re

# Get MQTT configuration from environment variables
MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")  # Default to 'localhost' if not set
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))  # Default port is 1883
MQTT_USERNAME = os.getenv("MQTT_USERNAME", "user")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "passwd")
MQTT_KEEP_ALIVE_INTERVAL = int(os.getenv("MQTT_KEEP_ALIVE_INTERVAL", 60))
MQTT_TOPIC_PREFIX = "homeassistant/sensor/tank_"
MQTT_CLIENT_ID = "tank_water_level_sensor"

# Serial Configuration
SERIAL_PORT = '/dev/serial0'
BAUD_RATE = 9600

# Tank height and water levels
height_of_tank = 190
min_level_of_water = 30
max_level_of_water = height_of_tank - 25

# Initialize the serial port
ser = serial.Serial(
    port=SERIAL_PORT,
    baudrate=BAUD_RATE,
    timeout=2,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    xonxoff=False,
    rtscts=False,
    dsrdtr=False,
    inter_byte_timeout=None,
    exclusive=False
)

data_pattern = re.compile(r'^\d+,\d+,\d+,\d+$')
latest_data = None

# Helper function to map values (e.g., for water level percentages)
def map_value(x, in_min, in_max, out_min, out_max):
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

# Read serial data in a separate thread
def read_serial_data():
    global latest_data
    while True:
        try:
            data = ser.readline()
            ser.reset_input_buffer()
            if data:
                decoded_data = data.decode('utf-8', errors='ignore').strip()
                print(f"decoded_data {decoded_data}")
                if data_pattern.match(decoded_data):
                    latest_data = decoded_data
        except serial.SerialException as e:
            print(f"Serial exception: {e}")
            ser.close()
            ser.open()
        except Exception as e:
            print(f"Error: {e}")

# Function to send data to MQTT and maintain heartbeat
def send_data_to_mqtt():

    while True:
        if latest_data:
            # Split and validate the data from the sensor
            sensor_id, distance, vcc, crc_sum = latest_data.split(',')
            sum = int(sensor_id) + int(distance) + int(vcc)
            if sum == int(crc_sum):
                # Calculate water level as percentage
                level_of_water = height_of_tank - int(distance)
                water_level_percent = map_value(level_of_water, min_level_of_water, max_level_of_water, 0, 100)
                water_level_percent = round(water_level_percent, 2)

                print(f"Publishing data to mqtt broker")

                # Publish the sensor data to MQTT, including the sensor_id
                mqtt_client.publish(f"{MQTT_TOPIC_PREFIX}water_level/state", 
                    json.dumps({"sensor_id": sensor_id, "water_level_percent": water_level_percent}), retain=False)
                mqtt_client.publish(f"{MQTT_TOPIC_PREFIX}vcc/state", 
                    json.dumps({"sensor_id": sensor_id, "vcc": vcc}), retain=False)
                mqtt_client.publish(f"{MQTT_TOPIC_PREFIX}distance/state", 
                    json.dumps({"sensor_id": sensor_id, "distance": distance}), retain=False)

        time.sleep(5)  # Sleep for a small amount to avoid excessive CPU usage

if __name__ == '__main__':
    # Initialize MQTT Client
    print(f"MQTT_BROKER {MQTT_BROKER} MQTT_PORT {MQTT_PORT} MQTT_USERNAME {MQTT_USERNAME} MQTT_PASSWORD {MQTT_PASSWORD}")
    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, MQTT_KEEP_ALIVE_INTERVAL)

    # Start the serial data reader in a separate thread
    threading.Thread(target=read_serial_data, daemon=True).start()

    # Start sending data to MQTT in a separate thread
    threading.Thread(target=send_data_to_mqtt, daemon=True).start()

    # Keep MQTT client loop running in the main thread
    mqtt_client.loop_forever()
