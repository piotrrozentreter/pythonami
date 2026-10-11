# LHA extract helpers for the Amiga GUI installer (shell out to lha).

import os


def drawer_dest(path):
    # Amiga LhA treats a bare last argument as a member name filter unless it
    # ends with ':' or '/' (destination drawer).
    if path == "":
        return path
    last = path[len(path) - 1]
    if last == ":" or last == "/":
        return path
    return path + "/"


def lha_extract(archive_path, dest_drawer):
    # Returns (ok, rc, message). ok is 1 on DOS RETURN_OK (0).
    # Match archive build flags: lha a -a -r …  →  lha x -a …
    dest = drawer_dest(dest_drawer)
    cmd = 'lha x -a "' + archive_path + '" "' + dest + '"'
    rc = os.system(cmd)
    if rc == 0:
        return (1, rc, "ok")
    return (0, rc, "lha failed with code " + str(rc))


def lha_available():
    rc = os.system(" whichtool lha >NIL: ")
    if rc == 0:
        return 1
    rc = os.system(" which lha >NIL: ")
    if rc == 0:
        return 1
    # Best-effort: try running lha with no args (often prints usage, rc may be non-zero).
    rc = os.system("lha >NIL:")
    if rc == 0 or rc == 5 or rc == 10:
        return 1
    return 0
