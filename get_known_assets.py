import os
import re

def find_pattern(path, r):
    res = []
    for root, _, files in os.walk(path):
        for file in files:
            with open(os.path.join(root, file), 'r', errors='ignore') as f:
                for m in re.finditer(r, f.read(), re.MULTILINE):
                    if len(m.group(1)) > 0:
                        res.append(m.group(1).strip())
    return list(set(res))

names = find_pattern('E:\\temp\\dumped_paks\\apex\\scripts', r"\$['\"](.*?)['\"]")
with open('known_assets.txt', 'w') as f:
    for name in names:
        f.write(name + "\n")
