#!/usr/bin/python3

import os
import datetime

tasks = {
    "P-LF-BST"       : os.getenv("DURINN_HOME") + "/benchmark/NVTraverse/BSTAravind/random/1000",
    "P-LF-Hash"      : os.getenv("DURINN_HOME") + "/benchmark/NVTraverse/Hash/random/1000",
    "P-LF-List"      : os.getenv("DURINN_HOME") + "/benchmark/NVTraverse/List/random/1000",
    "P-LF-Skiplist"  : os.getenv("DURINN_HOME") + "/benchmark/NVTraverse/Skiplist/random/1000",
    "P-LF-Queue"     : os.getenv("DURINN_HOME") + "/benchmark/PersistentQueue/random/1000",
    #
    "CCEH"           : os.getenv("DURINN_HOME") + "/benchmark/CCEH/random/1000",
    "FAST_FAIR"      : os.getenv("DURINN_HOME") + "/benchmark/FAST_FAIR/random/1000",
    #
    "P-ART"          : os.getenv("DURINN_HOME") + "/benchmark/RECIPE/P-ART/random/1000",
    "P-CLHT"         : os.getenv("DURINN_HOME") + "/benchmark/RECIPE/P-CLHT/random/1000",
    "P-HOT"          : os.getenv("DURINN_HOME") + "/benchmark/RECIPE/P-HOT/random/1000",
    "P-Masstree"     : os.getenv("DURINN_HOME") + "/benchmark/RECIPE/P-Masstree/random/1000",
    #
    "pmdk-array"     : os.getenv("DURINN_HOME") + "/benchmark/pmdk-examples/array/random/1000",
    "pmdk-queue"     : os.getenv("DURINN_HOME") + "/benchmark/pmdk-examples/queue/random/1000",
}

f = open('table-3.csv', 'w')
f.write('App, # stores, # LPs, # DL1 tests, # DL2 tests, Execution time\n')

def run(app_name, path):
    print(app_name + " starts...")

    os.chdir(path)
    os.system('make clean')
    os.system('make main.trace >/dev/null 2>/dev/null')

    t0 = datetime.datetime.now()
    os.system('make checker-output-ls6 >/dev/null 2>/dev/null')
    os.system('make checker-output6 >/dev/null 2>/dev/null')
    t1 = datetime.datetime.now()

    os.system('make res_all.csv')

    res(app_name, path, t1-t0)

    print(app_name + " ends.")

    os.system('rm -f /tmp/main.exe* /tmp/opt-*')

def res(app_name, path, t):
    f_res = open(path + '/res-output-ls/checker-output-ls6/res.txt')
    lines = f_res.readlines()
    stores = int(lines[1].split(':')[-1])
    lps = int(lines[2].split(':')[-1])

    f_DL3 = open(path + '/res-output/checker-output6/res.txt')
    DL3 = int(f_DL3.readlines()[4].split(':')[-1])

    res = '{}, {}, {}, {}, {}, {}, {}\n'.format(app_name, stores, lps, lps, lps, DL3, t)
    f.write(res)

def main():
    for name, path in tasks.items():
        run(name, path)

if __name__ == "__main__":
    main()
