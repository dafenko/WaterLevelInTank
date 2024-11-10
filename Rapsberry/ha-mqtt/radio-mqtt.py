import os
import paho.mqtt.client as mqtt
import time
import serial
import json
import threading
import re
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Get MQTT configuration from environment variables
MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))
MQTT_USERNAME = os.getenv("MQTT_USERNAME", "user")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "passwd")
MQTT_KEEP_ALIVE_INTERVAL = int(os.getenv("MQTT_KEEP_ALIVE_INTERVAL", 60))

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
latest_data = {}

# Dictionary to hold MQTT clients for each sensor
mqtt_clients = {}

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
                logging.info(f"Decoded data: {decoded_data}")
                
                # Validate and parse data
                if data_pattern.match(decoded_data):
                    sensor_id, distance, vcc, crc_sum = decoded_data.split(',')
                    calculated_crc = int(sensor_id) + int(distance) + int(vcc)
                    
                    # Validate CRC
                    if calculated_crc == int(crc_sum):
                        # Store validated data in the dictionary by sensor_id
                        latest_data[sensor_id] = {
                            "distance": int(distance),
                            "vcc": int(vcc),
                            "level_of_water": height_of_tank - int(distance)
                        }
                    else:
                        logging.error(f"CRC check failed for sensor {sensor_id}")
        except serial.SerialException as e:
            logging.error(f"Serial exception: {e}")
            ser.close()
            ser.open()
        except Exception as e:
            logging.error(f"Error: {e}")

# Initialize MQTT client for a specific sensor ID
def get_mqtt_client(sensor_id):
    if sensor_id in mqtt_clients:
        return mqtt_clients[sensor_id]
    
    # Create a unique client ID and topics for each sensor ID
    client_id = f"tank_water_level_sensor_{sensor_id}"
    client = mqtt.Client(client_id)
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.connect(MQTT_BROKER, MQTT_PORT, MQTT_KEEP_ALIVE_INTERVAL)
    client.loop_start()
    mqtt_clients[sensor_id] = client

    # Publish initial configuration for Home Assistant discovery
    register_sensor_in_homeassistant(client, sensor_id)

    return client

# Register the sensor in Home Assistant with config topics
def register_sensor_in_homeassistant(client, sensor_id):
    topic_prefix = f"homeassistant/sensor/tank_{sensor_id}_"
    config_payloads = {
       "water_level_percent": {
            "name": f"Tank {sensor_id} Water Level",
            "state_topic": f"{topic_prefix}water_level_percent/state",
            "unit_of_measurement": "%",
            "value_template": "{{ value_json.water_level_percent }}",
            "unique_id": f"tank_{sensor_id}_water_level_percent",
            "device_class": "humidity", 
        },
        "vcc": {
            "name": f"Tank {sensor_id} VCC",
            "state_topic": f"{topic_prefix}vcc/state",
            "unit_of_measurement": "V",
            "value_template": "{{ value_json.vcc }}",
            "unique_id": f"tank_{sensor_id}_vcc",
            "device_class": "voltage",
        },
        "distance": {
            "name": f"Tank {sensor_id} Distance",
            "state_topic": f"{topic_prefix}distance/state",
            "unit_of_measurement": "cm",
            "value_template": "{{ value_json.distance }}",
            "unique_id": f"tank_{sensor_id}_distance",
            "device_class": "distance",
        },
    }

    for key, payload in config_payloads.items():
        logging.info(f"Registered at mqtt {topic_prefix}{key}/config")
        config_topic = f"{topic_prefix}{key}/config"
        client.publish(config_topic, json.dumps(payload), retain=True)

# Function to send data to MQTT
def send_data_to_mqtt():
    while True:

        # Iterate over each sensor in latest_data
        for sensor_id, data in latest_data.items():
            client = get_mqtt_client(sensor_id)
            water_level_percent = map_value(
                data["level_of_water"], min_level_of_water, max_level_of_water, 0, 100
            )
            water_level_percent = round(water_level_percent, 2)
            
            # Define sensor-specific topics
            topic_prefix = f"homeassistant/sensor/tank_{sensor_id}_"

            # Publish each metric for the sensor
            client.publish(
                f"{topic_prefix}water_level_percent/state",
                json.dumps({"sensor_id": sensor_id, "water_level_percent": water_level_percent}),
                retain=False
            )
            logging.info(f"Publishing mqtt {topic_prefix}water_level_percent/state")
            client.publish(
                f"{topic_prefix}vcc/state",
                json.dumps({"sensor_id": sensor_id, "vcc": data["vcc"]}),
                retain=False
            )
            logging.info(f"Publishing mqtt {topic_prefix}vcc/state")
            client.publish(
                f"{topic_prefix}distance/state",
                json.dumps({"sensor_id": sensor_id, "distance": data["distance"]}),
                retain=False
            )
            logging.info(f"Publishing mqtt {topic_prefix}distance/state")

        time.sleep(5)  # Avoid excessive CPU usage

if __name__ == '__main__':
    # Start reading serial data in a separate thread
    threading.Thread(target=read_serial_data, daemon=True).start()

    # Start sending data to MQTT in a separate thread
    threading.Thread(target=send_data_to_mqtt, daemon=True).start()

    # Keep the script running
    while True:
        time.sleep(1)
