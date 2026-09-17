#!/usr/bin/env python3
import argparse
import datetime
import os
import re
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
CHANGELOG_PATH = os.path.join(REPO_ROOT, 'changelog.txt')
SET_VERSION_PATH = os.path.join(REPO_ROOT, 'Telegram', 'build', 'set_version.py')


def parse_version_string(ver_raw, force_beta=False, beta_num_arg=None):
    ver = ver_raw.strip()
    is_beta = force_beta or ('beta' in ver.lower())
    beta_num = beta_num_arg

    pattern = r'^\s*(\d+)\.(\d+)(?:\.(\d+))?(?:(?:\.|\s+|-)?(?:-)?beta(?:(?:\.|\s+)?(\d+))?|\.(\d+))?\s*$'
    match = re.match(pattern, ver, re.IGNORECASE)
    if not match:
        raise ValueError(f"Invalid version format: '{ver_raw}'. Expected format: X.Y, X.Y.Z, or X.Y.Z.beta N")

    major = match.group(1)
    minor = match.group(2)
    patch = match.group(3) if match.group(3) else '0'
    raw_beta_num = match.group(4)

    if is_beta:
        if raw_beta_num:
            beta_num = int(raw_beta_num)
        elif beta_num is None:
            beta_num = 1

    version_str = f"{major}.{minor}.{patch}"
    version_small = version_str if patch != '0' else f"{major}.{minor}"
    return version_str, version_small, is_beta, beta_num


def has_changelog_entry(content, version_str, version_small, is_beta, beta_num):
    for line in content.splitlines():
        line = line.strip()
        if is_beta:
            prefixes = [
                f"{version_str} beta {beta_num} ",
                f"{version_small} beta {beta_num} ",
                f"{version_str}.beta {beta_num} ",
                f"{version_small}.beta {beta_num} ",
            ]
            if any(line.startswith(p.strip()) for p in prefixes):
                return True
        else:
            if (line.startswith(version_str + ' ') or line.startswith(version_small + ' ')) and ' beta' not in line:
                return True
    return False


def build_entry_block(version_str, is_beta, beta_num, date_str, entries):
    if is_beta:
        header = f"{version_str} beta {beta_num} ({date_str})"
    else:
        header = f"{version_str} ({date_str})"
    lines = [header]
    if not entries:
        entries = ["Beta release"] if is_beta else ["Bug fixes and performance improvements"]

    for entry in entries:
        entry = entry.strip()
        if not entry.startswith('-') and not entry.startswith('—') and not entry.startswith('*'):
            entry = f"- {entry}"
        lines.append(entry)

    return "\n".join(lines) + "\n\n"


def update_changelog(version_str, version_small, is_beta, beta_num, date_str, entries, force=False):
    if not os.path.isfile(CHANGELOG_PATH):
        content = ""
    else:
        with open(CHANGELOG_PATH, 'r', encoding='utf-8') as f:
            content = f.read()

    already_exists = has_changelog_entry(content, version_str, version_small, is_beta, beta_num)
    target_name = f"{version_str} beta {beta_num}" if is_beta else version_str
    if already_exists and not force:
        print(f"Changelog entry for {target_name} already exists in changelog.txt")
        return False

    new_block = build_entry_block(version_str, is_beta, beta_num, date_str, entries)

    if already_exists and force:
        escaped_target = re.escape(target_name)
        pattern = rf"^{escaped_target}\s+.*?(?=\n[0-9]+\.[0-9]+|\Z)"
        content = re.sub(pattern, new_block.strip(), content, flags=re.DOTALL | re.MULTILINE)
        final_content = content
    else:
        final_content = new_block + content.lstrip()

    with open(CHANGELOG_PATH, 'w', encoding='utf-8', newline='\n') as f:
        f.write(final_content)

    print(f"Added changelog entry for {target_name} ({date_str}) to changelog.txt")
    return True


def main():
    parser = argparse.ArgumentParser(description="Update changelog.txt for version bumping")
    parser.add_argument("version", nargs="*", default=None,
                        help="Target version (e.g. 2.5.4, 2.5.4.beta 1)")
    parser.add_argument("-beta", "--beta", action="store_true",
                        help="Format entry as beta (e.g. '2.5.4 beta 1 (DD.MM.YY)')")
    parser.add_argument("-beta-num", "--beta-num", type=int, default=None,
                        help="Beta iteration number (default: 1)")
    parser.add_argument("-d", "--date", default=None,
                        help="Changelog date in DD.MM.YY format (default: today)")
    parser.add_argument("-e", "-m", "--entry", action="append", dest="entries",
                        help="Changelog bullet point (can be repeated)")
    parser.add_argument("--check", action="store_true",
                        help="Check if changelog entry exists and exit (0 if exists, 1 if missing)")
    parser.add_argument("-f", "--force", action="store_true",
                        help="Force update entry if it already exists")
    parser.add_argument("--bump", action="store_true",
                        help="Run set_version.py after updating changelog")
    args = parser.parse_args()

    ver_input = ' '.join(args.version).strip() if args.version else ''
    if not ver_input:
        version_file = os.path.join(REPO_ROOT, 'Telegram', 'build', 'version')
        if os.path.isfile(version_file):
            with open(version_file, 'r', encoding='utf-8') as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) >= 2 and parts[0] == 'AppVersionOriginal':
                        ver_input = ' '.join(parts[1:])
                        break

    if not ver_input:
        print("Error: Version must be provided or present in Telegram/build/version", file=sys.stderr)
        sys.exit(1)

    try:
        version_str, version_small, is_beta, beta_num = parse_version_string(
            ver_input, force_beta=args.beta, beta_num_arg=args.beta_num
        )
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    date_str = args.date or datetime.datetime.now().strftime("%d.%m.%y")
    target_label = f"{version_str} beta {beta_num}" if is_beta else version_str

    if args.check:
        if not os.path.isfile(CHANGELOG_PATH):
            sys.exit(1)
        with open(CHANGELOG_PATH, 'r', encoding='utf-8') as f:
            content = f.read()
        if has_changelog_entry(content, version_str, version_small, is_beta, beta_num):
            print(f"Changelog entry for {target_label} exists.")
            sys.exit(0)
        else:
            print(f"Changelog entry for {target_label} not found.", file=sys.stderr)
            sys.exit(1)

    update_changelog(version_str, version_small, is_beta, beta_num, date_str, args.entries, force=args.force)

    if args.bump:
        cmd = [sys.executable, SET_VERSION_PATH, version_str]
        if is_beta:
            cmd.extend(["-beta", str(beta_num)])
        print(f"Running: {' '.join(cmd)}")
        result = subprocess.run(cmd)
        sys.exit(result.returncode)


if __name__ == '__main__':
    main()
