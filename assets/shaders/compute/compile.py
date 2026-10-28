import os
from pathlib import Path
import subprocess

folders = [p for p in Path(".").iterdir() if p.is_dir()]
output_folder = "output"


for file in Path(".").rglob("*.comp"):
    output = os.path.join(
        output_folder,
        file.with_suffix(".spv").name
    )
    file = file.relative_to('.')
    cmd = [
        "glslangValidator",
        "-V",
        file,
        "-o",
        output
    ]

    print(f"Compiling {file} -> {output}")
    result = subprocess.run(cmd)

    if result.returncode != 0:
        print(f"Failed to compile {file}")