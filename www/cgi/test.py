#!/usr/bin/env python3
import os

value = os.environ["value"]

print("<h1>Addition Results</h1>")
try:
    num = int(value)
    content = f"<output>value = {num}</output>"
except:
    content = "<output>What the hell is this?</output>"

print("Content-Type: text/html")
print(f"Content-Length: {len(content)}")
print()
print(content)