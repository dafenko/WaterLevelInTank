from flask import Flask, jsonify, send_from_directory
import serial
import re
import threading

app = Flask(__name__)

# Initialize the serial port
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

height_of_tank = 190
min_level_of_water = 30
max_level_of_water = height_of_tank - 25

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
                print(f"decoded_data {decoded_data}")
                if data_pattern.match(decoded_data):
                    latest_data = decoded_data
        except serial.SerialException as e:
            print(f"Serial exception: {e}")
            ser.close()
            ser.open()
        except Exception as e:
            print(f"Error: {e}")

# Start the serial reading in a separate thread
threading.Thread(target=read_serial_data, daemon=True).start()

@app.route('/data', methods=['GET'])
def get_data():
    if latest_data:
        sensor_id, distance, vcc, crc_sum  = latest_data.split(',')
        sum = int(sensor_id) + int(distance) + int(vcc)
        print(f"sum {sum}")
        if sum == int(crc_sum) :
            level_of_water = height_of_tank - int(distance)
            water_level_percent = map_value(level_of_water, min_level_of_water, max_level_of_water, 0, 100)
            water_level_percent = round(int(water_level_percent), 2)
            return jsonify({
                'sensor_id': sensor_id,
                'water_level_percent': water_level_percent,
                'distance': distance,
                'vcc': vcc
            })
        else:
            print(f"CRC chech didn't passed {sensor_id} {distance} {crc_sum}")
    return jsonify({'error': 'No data available'})

@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
