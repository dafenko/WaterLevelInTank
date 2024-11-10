import serial
import re
import threading
import json
import paho.mqtt.client as mqtt
import time

# MQTT Configuration
MQTT_BROKER = 'somebroker'  # Replace with your MQTT broker address
MQTT_PORT = 1883
MQTT_TOPIC = 'homeassistant/sensor/tank_water_level'
MQTT_CLIENT_ID = 'tank_water_level_sensor'
MQTT_USERNAME = 'user'  # Replace with the username set in Mosquitto
MQTT_PASSWORD = 'password'  # Replace with the password set in Mosquitto

# Serial port configuration
ser = serial.Serial(
    port='/dev/serial0',
    baudrate=9600,
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

# Tank configuration
height_of_tank = 190
min_level_of_water = 30
max_level_of_water = height_of_tank - 25

# MQTT client setup
mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)  # Add username and password
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

def map_value(x, in_min, in_max, out_min, out_max):
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

def read_serial_data():
    global latest_data
    while True:
        try:
            data = ser.readline()
            ser.reset_input_buffer()
            if data:
                decoded_data = data.decode('utf-8', errors='ignore').strip()
                print(f"Decoded data: {decoded_data}")
                if data_pattern.match(decoded_data):
                    latest_data = decoded_data
                    process_data(latest_data)
        except serial.SerialException as e:
            print(f"Serial exception: {e}")
            ser.close()
            ser.open()
        except Exception as e:
            print(f"Error: {e}")

def process_data(data):
    sensor_id, distance, vcc, crc_sum = data.split(',')
    if int(sensor_id) + int(distance) + int(vcc) == int(crc_sum):
        level_of_water = height_of_tank - int(distance)
        water_level_percent = map_value(level_of_water, min_level_of_water, max_level_of_water, 0, 100)
        water_level_percent = round(water_level_percent, 2)
        
        # Publish water level
        mqtt_client.publish("homeassistant/sensor/tank_water_level/state", json.dumps({"water_level_percent": water_level_percent}), retain=True)

        # Publish VCC voltage
        mqtt_client.publish("homeassistant/sensor/tank_vcc/state", json.dumps({"vcc": vcc}), retain=True)

        # Publish distance
        mqtt_client.publish("homeassistant/sensor/tank_distance/state", json.dumps({"distance": distance}), retain=True)
    else:
        print(f"CRC check failed for data: {data}")

def publish_to_mqtt(sensor_id, water_level_percent, distance, vcc):
    payload = {
        'sensor_id': sensor_id,
        'water_level_percent': water_level_percent,
        'distance': distance,
        'vcc': vcc
    }
    mqtt_client.publish(MQTT_TOPIC + "/state", json.dumps(payload))
    print(f"Published to MQTT: {payload}")

def publish_mqtt_discovery():
    # Discovery for Water Level
    config_payload_water_level = {
        "name": "Tank Water Level",
        "state_topic": "homeassistant/sensor/tank_water_level/state",
        "value_template": "{{ value_json.water_level_percent }}",
        "unit_of_measurement": "%",
        "device_class": "humidity",
        "unique_id": "tank_water_level_sensor",
    }
    mqtt_client.publish("homeassistant/sensor/tank_water_level/config", json.dumps(config_payload_water_level), retain=True)

    # Discovery for VCC
    config_payload_vcc = {
        "name": "Tank VCC Voltage",
        "state_topic": "homeassistant/sensor/tank_vcc/state",
        "value_template": "{{ value_json.vcc }}",
        "unit_of_measurement": "V",
        "device_class": "voltage",
        "unique_id": "tank_vcc_sensor",
    }
    mqtt_client.publish("homeassistant/sensor/tank_vcc/config", json.dumps(config_payload_vcc), retain=True)

    # Discovery for Distance
    config_payload_distance = {
        "name": "Tank Distance",
        "state_topic": "homeassistant/sensor/tank_distance/state",
        "value_template": "{{ value_json.distance }}",
        "unit_of_measurement": "cm",
        "device_class": "distance",
        "unique_id": "tank_distance_sensor",
    }
    mqtt_client.publish("homeassistant/sensor/tank_distance/config", json.dumps(config_payload_distance), retain=True)

    print("Published MQTT Discovery Config for Water Level, VCC, and Distance")


# Publish MQTT discovery config for Home Assistant
publish_mqtt_discovery()

# Start the serial reading in a separate thread
threading.Thread(target=read_serial_data, daemon=True).start()

while True:
    time.sleep(1)
