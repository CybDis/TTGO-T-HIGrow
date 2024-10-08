'''
TTGO-HIGrow to MQTT Gateway, release 4.6.3
'''
import json
import yaml
import paho.mqtt.client as mqtt


# Configuration File Locations

SENSORS_FILE = "sensors/sensors.yaml"


# Write data to YAML file
def write_yaml_file(mac_id,  name):
    print("in write_yaml_file")
    sensors = dict()
#    print("after dict")
    sensors['mac_id'] = mac_id
    sensors['name'] = name

    with open(SENSORS_FILE, 'a') as yaml_file:
        yaml_file.write(yaml.safe_dump(sensors))

# Read data from YAML file
def read_yaml_file(mac_id,  name):
#    global mac_id_found,  data
    print('in read_yaml')
    with open(SENSORS_FILE, 'r',  encoding='utf-8') as yaml_file:
#        print("open sensor file")
        for line in yaml_file:
#            print("line ",  line)
            
            line_mac_id_input = line.strip()
            line_mac_id_arr = line_mac_id_input.split(": ")
            line_mac_id = line_mac_id_arr[1]
            if line_mac_id == mac_id:
#                print("found")
                mac_id_found = True
                break
        else:
            mac_id_found = False 
    
    yaml_file.close()        
    if mac_id_found == False:
         write_yaml_file(mac_id,  name)
         
    return mac_id_found

# Send discovery topics
def send_discovery_topics(msg):

    print("in send_discovery_topics")
    d = json.loads(msg.payload)
    device_payload = {
        'identifiers': [f"{d['plant']['Tgrow_HIGrow']}"],
        'manufacturer': "LILYGO, programmed by github.com/cybdis",
        'model':'TTGO T-Higrow',   
        'name':  d['plant']['sensorname'],
        'sw_version':  d['plant']['rel']
    }
    entity_payloads = {
        'sensorname': {
            'name': f"Name",
        },
        'Tgrow_HIGrow': {
            'name': f"Mac-ID",
        },
        'updated': {
            'name': f"Updated",
            'device_class': "timestamp", 
            'icon': 'mdi:update'
        },
        # 'date': {
        #     'name': f"Date",
        #     'icon': 'mdi:calendar'
        # },
        # 'time': {
        #     'name': f"Time",
        #     'icon': 'mdi:clock-outline', 
        #     'frc_upd':True
        # },
        'sleep5Count': {
            'name': f"Sleep5count",
            'icon':'mdi:counter'
        }, 
        'bootCount': {
            'name': f"Bootcount",
            'icon':'mdi:counter'
        }, 
        'lux': {
            'name': f"Lux",
            'unit_of_meas': "lx", 
            'state_class': "measurement",
            'device_class': "illuminance",
            'icon':'mdi:weather-sunny'
        }, 
        'temp': {
            'name': f"Temperature",
            'unit_of_meas': "°C", 
            'device_class': "temperature", 
            'state_class': "measurement",
            'icon':'mdi:thermometer'
        }, 
        'humid': {
            'name': f"Humidity",
            'unit_of_meas': "%", 
            'device_class': "humidity", 
            'state_class': "measurement",
            'icon':'mdi:water-percent'
        }, 
        'soil': {
            'name': f"Soil",
            'unit_of_meas': "%", 
            'icon':'mdi:raw',
            'state_class': "measurement",
            'device_class': 'moisture'
        }, 
        'soilRaw': {
            'name': f"SoilRaw",
            'icon':'mdi:water-percent',
            'state_class': "measurement"
        }, 
        'soilTemp': {
            'name': f"SoilTemp",
            'unit_of_meas': "°C", 
            'device_class': "temperature", 
            'state_class': "measurement",
            'icon':'mdi:thermometer',
        },
        'water': {
            'name': f"Water",
            'unit_of_meas': "%", 
            'state_class': "measurement",
            'icon':'mdi:waves-arrow-up'
        }, 
        'salt': {
            'name': f"Fertilizer",
            'unit_of_meas': "%", 
            'state_class': "measurement",
            'icon':'mdi:bottle-tonic'
        },
        'saltadvice': {
            'name': f"Fertilize state",
            'icon':'mdi:alpha-i-circle-outline'
        },
        'bat': {
            'name': f"Battery",
            'unit_of_meas': "%", 
            'device_class': "battery", 
            'state_class': "measurement",
            'icon':'mdi:battery'
        }, 
        'batcharge': {
            'name': f"Charging",
            'icon':'mdi:battery'
        }, 
        'batchargeDate': {
            'name': f"batchargeDate", 
            'icon':'mdi:calendar'
        }, 
        'daysOnBattery': {
            'name': f"daysOnBattery", 
            'unit_of_meas': "days", 
            'icon':'mdi:calendar'
        }, 
        'wifissid': {
            'name': f"WIFI",
            'icon':'mdi:wifi'
        }, 
        'pressure': {
            'name': f"Pressure",
            'unit_of_meas': "Hpa", 
            'state_class': "measurement",
            'icon':'mdi:gauge'
        }, 
        'plantValveNo': {
            'name': f"plantValveNo",
        }, 
        'rel': {
            'name': f"Release",
            'icon':'mdi:counter'
        }, 
    }
    print("1a")
    
    for entity, entity_payload in entity_payloads.items():
        entity_payload['val_tpl'] = f"{{{{ value_json.plant.{entity} }}}}"
        entity_payload['uniq_id'] = f"TTGO_{d['plant']['Tgrow_HIGrow']}_{entity}"
        entity_payload['stat_t'] =  f"{'Tgrow_HIGrow'}/{d['plant']['Tgrow_HIGrow']}"
        entity_payload['dev'] = device_payload
        sensor_type = ("sensor")
        entity_topic = f"{'homeassistant'}/{sensor_type}/{'Tgrow_HIGrow_'}{d['plant']['Tgrow_HIGrow']}/{entity}/config"
 
        print(json.dumps(entity_payload))
 
        client.publish(
            entity_topic,
            payload=json.dumps(entity_payload),
            qos=1, 
            retain=True
        )

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("Tgrow_HIGrow/#")

def on_message(client, userdata, msg):
    print("in on_message")
    print(msg.topic+" "+str(msg.payload))
    d = json.loads(msg.payload)
    mac_id=d["plant"]["Tgrow_HIGrow"]
    name=d["plant"]["sensorname"]
    yaml_data = read_yaml_file(mac_id,  name)

    if yaml_data == False:
        send_discovery_topics(msg)
        
    print("back in on_message after write")

# Initialize MQTT client connection
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
#client.username_pw_set("username", "password")
client.connect("192.168.1.6", 1883, 60)

client.loop_forever()
