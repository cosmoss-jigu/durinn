#!/usr/bin/python3

import os

class ResAll:
    def __init__(self):
        self.st_keys = ''
        self.mt_keys = ''
        self.f = open('res_all.csv', 'w')


    def get_res(self, workspace):
        keys = ''
        vals = ''
        with open(workspace+'/res.txt', 'r') as f:
            lines = f.read().split('\n')
            lines = lines[:-1]
            for line in lines:
                entry = line.split(':')
                key = entry[0]
                val = entry[1]
                keys += key + ','
                vals += val + ','
        return keys[:-1], vals[:-1]


    def get_res_st(self, workspace, name):
        workspace = 'res-output-ls/' + workspace
        keys, vals= self.get_res(workspace)
        if self.st_keys == '':
            self.st_keys = ',' + keys
            self.f.write(self.st_keys+'\n')
        self.f.write(name+','+vals+'\n')

    def get_res_mt(self, workspace, name):
        workspace = 'res-output/' + workspace
        keys, vals= self.get_res(workspace)
        if self.mt_keys == '':
            self.mt_keys = ',' + keys
            self.f.write(self.mt_keys+'\n')
        self.f.write(name+','+vals+'\n')

    def run(self):
        if os.path.isdir('checker-output-ls3'):
            self.get_res_st('checker-output-ls3', 'st-new')

        if os.path.isdir('checker-output-ls4'):
            self.get_res_st('checker-output-ls4', 'st-br')

        if os.path.isdir('checker-output-ls6'):
            self.get_res_st('checker-output-ls6', 'st-new-br')

        if os.path.isdir('checker-output-ls5'):
            self.get_res_st('checker-output-ls5', 'st-annotated-lps')

        self.f.write('\n')

        if os.path.isdir('checker-output3'):
            self.get_res_mt('checker-output3', 'mt-new')

        if os.path.isdir('checker-output4'):
            self.get_res_mt('checker-output4', 'mt-br')

        if os.path.isdir('checker-output6'):
            self.get_res_mt('checker-output6', 'mt-new-br')

        if os.path.isdir('checker-output-ls5'):
            self.get_res_mt('checker-output5', 'mt-annotated-lps')

def main():
    resall = ResAll()
    resall.run()

if __name__ == "__main__":
    main()
