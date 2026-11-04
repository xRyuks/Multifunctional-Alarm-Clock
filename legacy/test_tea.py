import json, urllib.request
u = "https://raw.githubusercontent.com/mathertel/Radio/master/src/TEA5767.cpp"
try:
    r = urllib.request.urlopen(u)
    code = r.read().decode('utf-8')
    for line in code.split('\n'):
        if "registers[" in line and "=" in line:
            print(line.strip())
except Exception as e:
    print(e)
