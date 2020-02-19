import socket
import random
import json
import paho.mqtt.client as mqtt

reading = 0

def on_message(client, userdata, message):
	print("MQTT received: {}".format(message.payload))
	global reading
	j = json.loads(message.payload)
	reading = j['value']
	print("Current reading: {}".format(reading))

def on_connect(client, userdata, flags, rc):
	if rc == 0:
		print("Connected to broker")
 
	else:
		print("Connection failed")

client = mqtt.Client("QWERT")
client.username_pw_set('maker:4MC9F0vopz86m0lqFyOE6RgK58yDtGoBsHff4Tr1', password='doesntmatter')
client.connect('api.allthingstalk.io')
client.on_connect = on_connect
client.on_message = on_message
print("MQTT connecting")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

server_address = '0.0.0.0'
server_port = 31337

server = (server_address, server_port)
sock.bind(server)

client.loop_start()
client.subscribe("asset/XYucFF14KKgx5weLVmDoN65h/feed")
print("Subscribed to MQTT")

print("Listening on " + server_address + ":" + str(server_port))


try:
	while True:
		payload, client_address = sock.recvfrom(1)
		rn = reading
		print("Sending {} back to {}".format(rn, str(client_address)))
		sent = sock.sendto((rn).to_bytes(2, byteorder='little'), client_address)
 
except KeyboardInterrupt:
	print("Exiting gracefully...")
	client.disconnect()
	client.loop_stop()