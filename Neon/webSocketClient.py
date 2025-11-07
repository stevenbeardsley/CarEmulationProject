import websocket
ws = websocket.create_connection('ws://localhost:8080')
try:
    while True:
        print(ws.recv())
except KeyboardInterrupt:
    ws.close()