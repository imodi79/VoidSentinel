"""Verify the real EXE icon resources and the Windows shell extraction path."""
import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import struct
import sys

path = str(Path(sys.argv[1]).resolve())
kernel = c.WinDLL("kernel32", use_last_error=True)
shell = c.WinDLL("shell32", use_last_error=True)
user = c.WinDLL("user32", use_last_error=True)
kernel.LoadLibraryExW.argtypes = [w.LPCWSTR, w.HANDLE, w.DWORD]
kernel.LoadLibraryExW.restype = w.HMODULE
kernel.FindResourceW.argtypes = [w.HMODULE, c.c_void_p, c.c_void_p]
kernel.FindResourceW.restype = w.HANDLE
kernel.SizeofResource.argtypes = [w.HMODULE, w.HANDLE]
kernel.SizeofResource.restype = w.DWORD
kernel.LoadResource.argtypes = [w.HMODULE, w.HANDLE]
kernel.LoadResource.restype = w.HANDLE
kernel.LockResource.argtypes = [w.HANDLE]
kernel.LockResource.restype = c.c_void_p
kernel.FreeLibrary.argtypes = [w.HMODULE]
shell.ExtractIconExW.argtypes = [w.LPCWSTR, c.c_int, c.POINTER(w.HICON), c.POINTER(w.HICON), w.UINT]
shell.ExtractIconExW.restype = w.UINT
user.DestroyIcon.argtypes = [w.HICON]

module = kernel.LoadLibraryExW(path, None, 0x22)
assert module, c.get_last_error()


def resource(identifier, kind):
    entry = kernel.FindResourceW(module, c.c_void_p(identifier), c.c_void_p(kind))
    assert entry
    pointer = kernel.LockResource(kernel.LoadResource(module, entry))
    assert pointer
    return c.string_at(pointer, kernel.SizeofResource(module, entry))


try:
    data = resource(101, 14)
    assert struct.unpack_from("<HHH", data) == (0, 1, 10)
    sizes = []
    for i in range(10):
        width, height, _, _, _, depth, length, identifier = struct.unpack_from("<BBBBHHIH", data, 6 + 14 * i)
        image = resource(identifier, 3)
        assert len(image) == length and image[:8] == b"\x89PNG\r\n\x1a\n"
        size = width or 256
        assert (height or 256) == size and depth == 32
        assert struct.unpack_from(">II", image, 16) == (size, size)
        sizes.append(size)
    assert sizes == [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
finally:
    kernel.FreeLibrary(module)

large, small = w.HICON(), w.HICON()
assert shell.ExtractIconExW(path, -1, None, None, 0) == 1
extracted = shell.ExtractIconExW(path, 0, c.byref(large), c.byref(small), 1)
assert 0 < extracted < 0xFFFFFFFF
assert large.value and small.value
user.DestroyIcon(large)
user.DestroyIcon(small)
print("PASS: ten embedded icon sizes and Windows shell large/small icon extraction")
