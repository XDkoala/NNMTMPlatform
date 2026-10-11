"""Fetch LICENSE/NOTICE only from an official PyTorch wheel using HTTP ranges.

The output is a small ZIP, not an installable wheel. Its unmodified license
entries can be consumed by Package-Windows.ps1 without downloading gigabytes.
"""
import argparse
import io
import urllib.request
import zipfile


class RemoteZip(io.RawIOBase):
    def __init__(self, url):
        self.url = url
        with urllib.request.urlopen(urllib.request.Request(url, method="HEAD"), timeout=60) as response:
            self.length = int(response.headers["Content-Length"])
        self.position = 0

    def seekable(self):
        return True

    def tell(self):
        return self.position

    def seek(self, offset, whence=0):
        self.position = offset + (0 if whence == 0 else self.position if whence == 1 else self.length)
        return self.position

    def read(self, size=-1):
        size = min(size if size >= 0 else self.length, self.length - self.position)
        if size <= 0:
            return b""
        request = urllib.request.Request(self.url, headers={
            "Range": f"bytes={self.position}-{self.position + size - 1}",
            "Accept-Encoding": "identity",
        })
        with urllib.request.urlopen(request, timeout=60) as response:
            if response.status != 206:
                raise RuntimeError("Server did not honor HTTP Range")
            expected = f"bytes {self.position}-{self.position + size - 1}/{self.length}"
            if response.headers.get("Content-Range") != expected:
                raise RuntimeError("Server returned an unexpected byte range")
            data = response.read(size)
        if len(data) != size:
            raise RuntimeError("Truncated HTTP Range")
        self.position += len(data)
        return data


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output")
    args = parser.parse_args()
    url = "https://download.pytorch.org/whl/cu128/torch-2.9.0%2Bcu128-cp311-cp311-win_amd64.whl"
    with zipfile.ZipFile(RemoteZip(url)) as upstream, zipfile.ZipFile(args.output, "x", zipfile.ZIP_DEFLATED) as output:
        entries = [name for name in upstream.namelist()
                   if ".dist-info/" in name and name.rsplit("/", 1)[-1] in ("LICENSE", "NOTICE")]
        if len(entries) != 2:
            raise RuntimeError("Expected exactly one LICENSE and NOTICE")
        for name in entries:
            data = upstream.read(name)  # zipfile also checks the upstream CRC.
            output.writestr(name, data)
            print(f"{name}: {len(data)} bytes")
    print(f"Source: {url}")
