#!/usr/bin/env python3
import re


def extract_version(header_file):
    version_parts = {"MAJOR": 0, "MINOR": 0, "RELEASE": 0}

    with open(header_file, "r") as file:
        content = file.read()
        for part in version_parts.keys():
            match = re.search(rf"#define CMP_VERSION_{part}\s+(\d+)", content)
            if match:
                version_parts[part] = int(match.group(1))
                continue
            raise Exception("Unable to find " + part + " version string")

    return (
        f"{version_parts['MAJOR']}.{version_parts['MINOR']}.{version_parts['RELEASE']}"
    )


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Print cmp version from lib/cmp.h")
    parser.add_argument("file", help="path to lib/cmp.h")
    args = parser.parse_args()
    version = extract_version(args.file)
    print(version)


if __name__ == "__main__":
    try:
        main()
    except Exception:
        print("unknown")
