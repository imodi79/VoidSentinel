import csv
from pathlib import Path
import sys

path = Path(sys.argv[1])
with path.open(encoding="utf-8-sig", newline="") as stream:
    rows = list(csv.DictReader(stream))
assert len(rows) == 1
assert rows[0]["Path"] == 'C:\\árvíz,"teszt"\n.txt'
assert rows[0]["Count"] == "1"
print("PASS: CSV Unicode, quote, comma, newline round-trip")
