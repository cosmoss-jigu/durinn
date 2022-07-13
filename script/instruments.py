#!/usr/bin/python3

import os
from concurrent.futures import ProcessPoolExecutor, wait
from tasks import tasks

make_cmds = {
    "P-LF-BST"       : "make include lib main",
    "P-LF-Hash"      : "make include lib main",
    "P-LF-List"      : "make include lib main",
    "P-LF-Skiplist"  : "make include lib main",
    "P-LF-Queue"     : "make include lib main",
    #
    "CCEH"           : "make util src main",
    "FAST_FAIR"      : "make include lib main",
    #
    "P-ART"          : "make include lib main",
    "P-CLHT"         : "make include lib main",
    "P-HOT"          : "make include main",
    "P-Masstree"     : "make include lib main",
    #
    "pmdk-array"     : "make main",
    "pmdk-queue"     : "make main",
}


def get_inst_cmd(app_name, path):
    path = path + "/../.."
    cmd = "cd " + path + ";"
    cmd += make_cmds[app_name]
    return cmd

def main():
    cmds = []
    for name, path in tasks.items():
        cmds.append(get_inst_cmd(name, path))
    futures = []
    pool_executor = ProcessPoolExecutor(max_workers=os.cpu_count()-1)
    for cmd in cmds:
        futures.append(pool_executor.submit(os.system, cmd))
    wait(futures)

if __name__ == "__main__":
    main()
