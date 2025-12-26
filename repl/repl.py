#!/usr/bin/env python3

import argparse
import sys
import serial
from serial.tools import list_ports


class SCPISerialSession:
  def __init__(self, port, baudrate=115200, timeout=1.0, eol="\r\n"):
    self._port = port
    self._baudrate = baudrate
    self._timeout = timeout
    self._eol = eol
    self._ser = None

  def open(self):
    self._ser = serial.Serial(
      port=self._port,
      baudrate=self._baudrate,
      bytesize=serial.EIGHTBITS,
      parity=serial.PARITY_NONE,
      stopbits=serial.STOPBITS_ONE,
      timeout=self._timeout
    )

  def close(self):
    if self._ser and self._ser.is_open:
      self._ser.close()

  def write(self, cmd):
    payload = (cmd + self._eol).encode("ascii", errors="ignore")
    self._ser.write(payload)

  def read_line(self):
    data = self._ser.readline()
    if not data:
      return None
    return data.decode("ascii", errors="replace").strip()

  def query(self, cmd):
    self.write(cmd)
    return self.read_line()

  def check_error(self):
    return self.query("SYST:ERR?")


def list_com_ports():
  ports = list_ports.comports()

  if not ports:
    print("No serial ports found.")
    return

  for p in ports:
    line = p.device
    if p.description:
      line += f" - {p.description}"
    if p.vid is not None and p.pid is not None:
      line += f" (VID={p.vid:04x}, PID={p.pid:04x})"
    if p.manufacturer:
      line += f" [{p.manufacturer}]"
    print(line)


def scpi_repl(session, auto_error_check=False):
  print("SCPI interactive console")
  print("  - Queries end with '?'")
  print("  - Empty line exits")
  print("  - Raw ASCII over serial")
  print("-" * 60)

  try:
    while True:
      try:
        cmd = input("SCPI> ").strip()
      except EOFError:
        break

      if not cmd:
        break

      if cmd.endswith("?"):
        response = session.query(cmd)
        if response is None:
          print("RX: <no response>")
        else:
          print("RX:", response)
      else:
        session.write(cmd)
        print("TX: OK")

        if auto_error_check:
          err = session.check_error()
          if err and not err.startswith("0"):
            print("ERR:", err)

  finally:
    session.close()
    print("Disconnected.")


def main():
  parser = argparse.ArgumentParser(
    description="SCPI REPL over serial (OWON XDM1041 compatible)"
  )

  parser.add_argument(
    "--list",
    action="store_true",
    help="List available COM ports and exit"
  )

  parser.add_argument(
    "--port",
    help="COM port (e.g. COM7)"
  )

  parser.add_argument(
    "--baudrate",
    type=int,
    default=115200,
    help="Baudrate (default: 115200)"
  )

  parser.add_argument(
    "--timeout",
    type=float,
    default=1.0,
    help="Read timeout in seconds (default: 1.0)"
  )

  parser.add_argument(
    "--auto-error",
    action="store_true",
    help="Automatically query SYST:ERR? after commands"
  )

  args = parser.parse_args()

  if args.list:
    list_com_ports()
    return

  if not args.port:
    parser.error("--port is required unless --list is used")

  session = SCPISerialSession(
    port=args.port,
    baudrate=args.baudrate,
    timeout=args.timeout
  )

  try:
    session.open()
  except serial.SerialException as e:
    print(f"Failed to open {args.port}: {e}")
    sys.exit(1)

  print(f"Connected to {args.port} @ {args.baudrate} baud")
  scpi_repl(session, auto_error_check=args.auto_error)


if __name__ == "__main__":
  main()
