"""Validate CSV captures produced by VoidSentinel --capture-test DIRECTORY.

Uses only the Python standard library; Python is a development-only test tool.
"""
import csv
import sys
from pathlib import Path


def rows(directory, name):
    with (directory / name.replace('.db', '.csv')).open(encoding='utf-8-sig',newline='') as f:
        values=list(csv.DictReader(f))
    keys=[(r['Application path'].lower(),r['Path'].lower(),r['Operation'].lower()) for r in values]
    assert len(keys)==len(set(keys)), 'duplicate app/path/operation'
    order=lambda r:(r['Application'].lower(),r['Application path'].lower(),{'Folder':0,'File':1,'Unknown':2}[r['Type']],r['Path'].lower(),r['Operation'].lower())
    assert values==sorted(values,key=order), 'wrong result ordering'
    return [(r['Application path'],r['Operation'],r['Path'],r['Result']) for r in values]


def verify_operations(events, marker):
    successful = [(operation, path.lower()) for _, operation, path, result in events
                  if result == "Success" and marker in path.lower()]
    for operation in ("Create", "Write", "End of file", "Rename request", "Delete request"):
        assert any(op == operation for op, _ in successful), (marker, operation)
    assert any("árvíztűrő-test.txt" in path for _, path in successful)
    assert any(op == "Delete request" and path.endswith("renamed.txt") for op, path in successful)
    assert any(op == "Delete request" and path.rstrip("\\").endswith(marker.rstrip("\\"))
               for op, path in successful), "directory delete missing"


directory = Path(sys.argv[1]).resolve()
with_children = rows(directory, "capture.db")
verify_operations(with_children, "\\probe\\root")
verify_operations(with_children, "\\probe\\child")
assert any(app.lower().endswith("child-helper.exe") for app, *_ in with_children)
assert not any(app.lower().endswith("unrelated-helper.exe") for app, *_ in with_children)
assert not any("\\unrelated\\" in path.lower() for _, _, path, _ in with_children)

without_children = rows(directory, "no-children.db")
verify_operations(without_children, "\\second\\root")
assert not any(app.lower().endswith("child-helper.exe") for app, *_ in without_children)
assert not any("\\second\\child\\" in path.lower() for _, _, path, _ in without_children)

stress = rows(directory, "stress.db")
with (directory/'stress.csv').open(encoding='utf-8-sig',newline='') as f:
    writes=[r for r in csv.DictReader(f) if r['Operation']=='Write' and r['Path'].lower().endswith('\\stress.bin')]
assert len(writes)==1, f"Expected a single aggregated write row, got {len(writes)}"
assert int(writes[0]['Count'])>=10000
assert any(app.lower().endswith("unrelated-helper.exe") for app, *_ in stress), "second explicit target missing"
verify_operations(stress, "\\explicit-target")
print("PASS: Unicode file/directory operations, child inclusion/exclusion, unrelated process exclusion,")
print("      multiple targets, restart, 10000 writes, CSV ordering, per-operation aggregation.")

with (directory / 'meaningful.csv').open(encoding='utf-8-sig', newline='') as stream:
    summary = list(csv.DictReader(stream))
shared = [row for row in summary if row['Path'].endswith('\\shared.txt')]
assert len(shared) == 1
assert 'Created' in shared[0]['Operation'] and 'Written' in shared[0]['Operation']
assert 'child-helper.exe' in shared[0]['Process chain']
assert shared[0]['Application path'] == shared[0]['Root application']
saved = [row for row in summary if row['Path'].endswith('\\saved-document.txt')]
assert len(saved) == 1 and 'Saved via rename' in saved[0]['Operation']
assert saved[0]['Original path'].endswith('\\atomic-save.tmp')
assert not any(row['Path'].endswith(('\\atomic-save.tmp', '\\ephemeral.tmp')) for row in summary)
assert not any(row['Path'].startswith('[unresolved') for row in summary)
assert all(int(row['Failures']) == 0 and int(row['Unconfirmed']) == 0 for row in summary)
with (directory / 'full.csv').open(encoding='utf-8-sig', newline='') as stream:
    full = list(csv.DictReader(stream))
assert any(row['Path'].endswith('\\ephemeral.tmp') for row in full)
assert any(int(row['Failures']) > 0 for row in full)
assert any(row['Destination'].endswith('\\saved-document.txt') for row in full)
print('PASS: root/child summaries, successful-only view, observed rename destination, atomic save,')
print('      transient filtering, unresolved path filtering, full-footprint recovery of hidden records.')
with (directory/'worker-meaningful.csv').open(encoding='utf-8-sig',newline='') as stream:
    worker=list(csv.DictReader(stream))
assert any(row['Path'].endswith('\\shared.txt') and 'child-helper.exe' in row['Process chain'] for row in worker)
assert any(row['Path'].endswith('\\saved-document.txt') for row in worker)
assert not any(row['Path'].endswith('\\ephemeral.tmp') for row in worker)
print('PASS: background session start/stop, final publication, live ETW semantic snapshot.')
