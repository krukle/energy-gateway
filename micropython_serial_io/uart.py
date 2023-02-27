import random
import time
from machine import UART

uart = UART(2, baudrate=15200, bits=8, parity=None, stop=1, timeout=30)

def send(data: int, verbose: bool = True) -> int:
    """Send data to the serial port."""
    if verbose: print(f"Sending data: {data}...")
    numBytesWritten = uart.write(data.to_bytes(1, "big"))
    if numBytesWritten and verbose: print("Data sent")
    if not numBytesWritten and verbose: print("Data not sent")
    return numBytesWritten

def receive(verbose: bool = True) -> bytes:
    """Receive data from the serial port."""
    if verbose: print("Receiving data...") 
    received = uart.read()
    if received and verbose: 
        print(f"Data received: {received}")
        time.sleep(2)
    if not received and verbose: print("No data received")
    return received

while True:
    send(random.randint(0, 255))
    receive()
    print()
    