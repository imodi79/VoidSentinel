"""Package supplied PNGs into an ICO without resampling. Run only after PNG changes."""
from pathlib import Path
import struct

root = Path(__file__).resolve().parent
sizes = (16, 20, 24, 32, 40, 48, 64, 96, 128, 256)
frames = [(size, (root / f"VoidSentinel_{size}.png").read_bytes()) for size in sizes]
offset = 6 + 16 * len(frames)
directory = bytearray(struct.pack("<HHH", 0, 1, len(frames)))
for size, data in frames:
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    assert struct.unpack_from(">II", data, 16) == (size, size)
    directory.extend(struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32, len(data), offset))
    offset += len(data)
(root / "VoidSentinel.windows.ico").write_bytes(directory + b"".join(data for _, data in frames))
