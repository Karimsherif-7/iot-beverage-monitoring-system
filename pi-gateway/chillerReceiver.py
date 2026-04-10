#!/usr/bin/env python3
"""
chillerReceiver.py
Bare-bones Pi 5 LoRa receiver using the SX1262 radio over direct SPI (lgpio).
Prints all received packets to stdout. No InfluxDB integration.

Use this script to verify the radio link is working before enabling
data logging. This was the first script used on the Pi side after
pingpong testing between two LoRa32 boards was confirmed working.

Radio config: 915 MHz, BW=125 kHz, SF=7, CR=5
Pins: CS=21, BUSY=24, RST=22, DIO1=16, TXEN=6
"""
import time
import spidev
import lgpio

PIN_CS   = 21
PIN_BUSY = 24
PIN_RST  = 22
PIN_DIO1 = 16
PIN_TXEN = 6

FREQUENCY = 915.0
BANDWIDTH = 125.0
SF = 7
CR = 5
SYNC_WORD = 0x12
PREAMBLE = 8

BW_MAP = {125.0: 0x04, 250.0: 0x05, 500.0: 0x06}

class SX1262:
    def __init__(self):
        self.h = lgpio.gpiochip_open(0)
        lgpio.gpio_claim_output(self.h, PIN_CS, 1)
        lgpio.gpio_claim_output(self.h, PIN_RST, 1)
        lgpio.gpio_claim_input(self.h, PIN_BUSY)
        lgpio.gpio_claim_input(self.h, PIN_DIO1)
        lgpio.gpio_claim_output(self.h, PIN_TXEN, 0)
        self.spi = spidev.SpiDev()
        self.spi.open(0, 0)
        self.spi.max_speed_hz = 2000000
        self.spi.mode = 0
        self.spi.no_cs = True

    def _busy(self):
        t = time.time()
        while lgpio.gpio_read(self.h, PIN_BUSY):
            if time.time() - t > 1: raise TimeoutError("BUSY")
            time.sleep(0.001)

    def _cmd(self, data, read=0):
        self._busy()
        lgpio.gpio_write(self.h, PIN_CS, 0)
        if read:
            r = self.spi.xfer2(list(data) + [0]*read)
            lgpio.gpio_write(self.h, PIN_CS, 1)
            return r[len(data):]
        self.spi.xfer2(list(data))
        lgpio.gpio_write(self.h, PIN_CS, 1)

    def _wreg(self, addr, data):
        self._busy()
        lgpio.gpio_write(self.h, PIN_CS, 0)
        self.spi.xfer2([0x0D, (addr>>8)&0xFF, addr&0xFF] + list(data))
        lgpio.gpio_write(self.h, PIN_CS, 1)

    def init(self):
        lgpio.gpio_write(self.h, PIN_RST, 0)
        time.sleep(0.01)
        lgpio.gpio_write(self.h, PIN_RST, 1)
        time.sleep(0.05)
        self._busy()
        self._cmd([0x80, 0x00])
        self._cmd([0x96, 0x01])
        self._cmd([0x89, 0x7F])
        time.sleep(0.01)
        self._cmd([0x9D, 0x01])
        self._cmd([0x8A, 0x01])
        freq = int((FREQUENCY * 1e6 * (2**25)) / 32e6)
        self._cmd([0x86, (freq>>24)&0xFF, (freq>>16)&0xFF, (freq>>8)&0xFF, freq&0xFF])
        self._cmd([0x95, 0x04, 0x07, 0x00, 0x01])
        self._cmd([0x8E, 10, 0x04])
        self._cmd([0x8B, SF, BW_MAP[BANDWIDTH], CR-4, 0x00])
        self._cmd([0x8C, 0x00, PREAMBLE, 0x00, 0xFF, 0x00, 0x00])
        self._wreg(0x0740, [0x14, 0x24])
        self._cmd([0x8F, 0x00, 0x80])
        irq = 0x0202
        self._cmd([0x08, (irq>>8)&0xFF, irq&0xFF, (irq>>8)&0xFF, irq&0xFF, 0,0, 0,0])
        print("Radio ready!")

    def receive(self):
        self._cmd([0x82, 0xFF, 0xFF, 0xFF])
        print("Listening...\n")
        while True:
            if lgpio.gpio_read(self.h, PIN_DIO1):
                irq = self._cmd([0x12], read=3)
                status = (irq[1]<<8) | irq[2]
                self._cmd([0x02, (status>>8)&0xFF, status&0xFF])
                if status & 0x0002:
                    buf = self._cmd([0x13], read=3)
                    plen, offset = buf[1], buf[2]
                    if 0 < plen < 250:
                        raw = self._cmd([0x1E, offset], read=plen+1)
                        msg = bytes(raw[1:plen+1]).decode('utf-8', errors='ignore').strip('\x00')
                        print(f"RX: {msg}")
                    self._cmd([0x82, 0xFF, 0xFF, 0xFF])
                elif status & 0x0200:
                    self._cmd([0x82, 0xFF, 0xFF, 0xFF])
            time.sleep(0.05)

    def cleanup(self):
        self._cmd([0x80, 0x00])
        self.spi.close()
        lgpio.gpiochip_close(self.h)

if __name__ == "__main__":
    r = SX1262()
    try:
        r.init()
        r.receive()
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        r.cleanup()
