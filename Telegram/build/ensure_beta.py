#!/usr/bin/env python3
import os
import re
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
VERSION_FILE = os.path.join(REPO_ROOT, 'Telegram', 'build', 'version')
CORE_VERSION_H = os.path.join(REPO_ROOT, 'Telegram', 'SourceFiles', 'core', 'version.h')
FA_VERSION_H = os.path.join(REPO_ROOT, 'Telegram', 'SourceFiles', 'fa', 'fa_version.h')


def update_file(path, replacements):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()

    modified = content
    for pattern, repl in replacements:
        modified = re.sub(pattern, repl, modified)

    if modified != content:
        with open(path, 'w', encoding='utf-8', newline='\n') as f:
            f.write(modified)
        return True
    return False


def ensure_beta():
    changed = False

    if os.path.isfile(VERSION_FILE):
        with open(VERSION_FILE, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        new_lines = []
        has_beta_number = False
        for line in lines:
            if re.match(r'^BetaChannel\s+\d', line):
                new_lines.append('BetaChannel        1\n')
                if not re.match(r'^BetaChannel\s+1', line):
                    changed = True
            elif re.match(r'^BetaNumber\s+\d', line):
                has_beta_number = True
                parts = line.strip().split()
                if len(parts) > 1 and parts[1] == '0':
                    new_lines.append('BetaNumber         1\n')
                    changed = True
                else:
                    new_lines.append(line)
            elif re.match(r'^AppVersionOriginal\s+', line):
                parts = line.strip().split(maxsplit=1)
                orig_ver = parts[1] if len(parts) > 1 else ''
                if orig_ver and 'beta' not in orig_ver.lower():
                    new_ver = orig_ver + '.beta 1'
                    new_lines.append(f'AppVersionOriginal {new_ver}\n')
                    changed = True
                else:
                    new_lines.append(line)
            else:
                new_lines.append(line)

        if not has_beta_number:
            # insert after BetaChannel if found
            inserted = False
            final_lines = []
            for line in new_lines:
                final_lines.append(line)
                if line.startswith('BetaChannel') and not inserted:
                    final_lines.append('BetaNumber         1\n')
                    inserted = True
                    changed = True
            new_lines = final_lines

        with open(VERSION_FILE, 'w', encoding='utf-8', newline='\n') as f:
            f.writelines(new_lines)

    if os.path.isfile(CORE_VERSION_H):
        if update_file(CORE_VERSION_H, [
            (r'constexpr auto AppBetaVersion = (false|true);', 'constexpr auto AppBetaVersion = true;'),
            (r'constexpr auto AppBetaNumber = 0;', 'constexpr auto AppBetaNumber = 1;')
        ]):
            changed = True

    if os.path.isfile(FA_VERSION_H):
        if update_file(FA_VERSION_H, [
            (r'constexpr auto AppFABetaVersion = (false|true);', 'constexpr auto AppFABetaVersion = true;'),
            (r'constexpr auto AppFABetaNumber = 0;', 'constexpr auto AppFABetaNumber = 1;')
        ]):
            changed = True

    print("Beta build configuration ensured.")
    return changed


if __name__ == '__main__':
    ensure_beta()
