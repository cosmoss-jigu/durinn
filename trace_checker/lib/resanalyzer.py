import os
import pickle
import shutil
from lib.pickle import PICKLE_GDB_RES
from lib.filenames import RACE_OP
from lib.trace import Trace

SRC_MAP = 'src_map.txt'

class ResAnalyzer:
    def __init__(self, workspace, output, opt, tracerdir, allbc):
        self.init_workspace(workspace, output)
        self.get_res_stats()

        self.opt = opt
        self.tracerdir = tracerdir
        self.allbc = allbc

        self.res_map = dict()

        self.grep_gdb_res()
        self.print_summary()
        self.print_samples()
        self.print_race_op_summary()

    def init_workspace(self, workspace, output):
        self.workspace = workspace
        self.output = output + '/' + workspace
        self.res_file = self.output + '/res.txt'
        if not os.path.isdir(output):
            os.mkdir(output)
        os.mkdir(self.output)
        shutil.copy(workspace+'/res.txt', self.output)

    def get_res_stats(self):
        res = dict()

        for d in os.listdir(self.workspace):
            path = os.path.join(self.workspace, d)
            if not os.path.isdir(path):
                continue
            if d.count('-') != 1:
                continue
            if not os.path.exists(path+'/res.txt'):
                continue
            with open(path+'/res.txt', 'r') as f:
                lines = f.read().split('\n')
                lines = lines[:-1]
                for line in lines:
                    entry = line.split(':')
                    key = entry[0]
                    val = int(entry[1])
                    if key not in res:
                        res[key] = 0
                    res[key] += val
        with open(self.output+'/res.txt', 'a') as f:
            for key in res:
                f.write(key + ':' + str(res[key]) + '\n')

    def grep_gdb_res(self):
        for d in os.listdir(self.workspace):
            path = os.path.join(self.workspace, d)
            if not os.path.isdir(path):
                continue
            if d.count('-') != 2:
                continue
            gdb_res_pickle_path = path + '/' + PICKLE_GDB_RES
            # TODO
            #assert(os.path.isfile(gdb_res_pickle_path))
            if os.path.isfile(gdb_res_pickle_path):
                self.gdb_res_pickle_analysis(gdb_res_pickle_path)

    def gdb_res_pickle_analysis(self, path):
        gdb_res = pickle.load(open(path, 'rb'))
        sig = gdb_res.trace_signature
        if sig not in self.res_map:
            self.res_map[sig] = []
        self.res_map[sig].append(gdb_res)

    def print_summary(self):
        f_summary = open(self.output+'/res_summary', 'w')
        for sig in self.res_map:
            f_summary.write('trace_signature: ' + sig + '\n')
            for gdb_res in self.res_map[sig]:
                f_summary.write('\t' + gdb_res.workspace + '\t' + gdb_res.buggy_type + '\n')
            f_summary.write('\n')
        f_summary.close()

    def print_samples(self):
        self.iids = set()
        self.dir_to_trace = dict()
        for sig in self.res_map:
            gdb_res_sample = self.res_map[sig][0]
            self.print_sample_prologue(gdb_res_sample)

        self.gen_src_mapping()

        for sample_dir in self.dir_to_trace:
            self.print_sample_epilogue(sample_dir, self.dir_to_trace[sample_dir])

    def print_sample_prologue(self, gdb_res):
        sample_dir = self.workspace + '/' + gdb_res.workspace
        sample_trace_file = sample_dir + '/' + gdb_res.trace_file
        sample_race_op_file = sample_dir + '/' + RACE_OP

        output_dir = self.output + '/' + gdb_res.workspace
        os.makedirs(output_dir)

        shutil.copy(sample_trace_file, output_dir)
        #shutil.copy(sample_race_op_file, output_dir)
        #shutil.copy(sample_dir+'/*.txt', output_dir)
        cmd = 'cp ' + sample_dir + '/*.txt ' + output_dir
        os.system(cmd)

        trace = Trace(sample_trace_file)
        self.dir_to_trace[output_dir] = trace

        assert(len(trace.ops_api_ranges) == 2)
        for op in trace.ops[trace.ops_api_ranges[0][0]:trace.ops_api_ranges[1][1]+1]:
            if op.src_info == 'NA':
                continue
            self.iids.add(op.src_info)

    def gen_src_mapping(self):
        if len(self.iids) == 0:
            return

        cmd = [self.opt,
               '-load ' + self.tracerdir+'/libciri.so',
		       '-mergereturn -lsnum',
               '-dfencesrcmap -fence-ids',
               ','.join(self.iids),
               '-output',
               self.output + '/' + SRC_MAP,
               '-remove-lsnum -stats ' + self.allbc,
               '-o /dev/null']
        print(' '.join(cmd));
        os.system(' '.join(cmd))

        self.src_map = dict()
        entries = open(self.output+'/'+SRC_MAP).read().split("\n")[:-1]
        for entry in entries:
            strs = entry.split(',')
            inst_id = strs[0]
            src_info = strs[1]
            self.src_map[inst_id] = src_info

    def print_sample_epilogue(self, directory, trace):
        f_trace = open(directory+'/sample.src.trace', 'w')
        for op in trace.ops[trace.ops_api_ranges[0][0]:trace.ops_api_ranges[1][1]+1]:
            if op.src_info != 'NA':
                op.src_info = self.src_map[op.src_info]
            f_trace.write(str(op) + '\n')
        f_trace.close()

    def print_race_op_summary(self):
        race_op_type_map = dict()
        for sig in self.res_map:
            gdb_res = self.res_map[sig][0]
            race_op_type = gdb_res.race_op_type
            if race_op_type.race_type not in race_op_type_map:
                race_op_type_map[race_op_type.race_type] = []
            race_op_type_map[race_op_type.race_type].append(gdb_res)

        f_summary = open(self.output+'/res_race_op_summary', 'w')
        for race_type in race_op_type_map:
            f_summary.write('race_op_type: ' + race_type + '\n')
            for gdb_res in race_op_type_map[race_type]:
                f_summary.write('\t' + gdb_res.workspace +
                                '\t' + gdb_res.race_op_type.race_type +
                                '\t' + str(gdb_res.race_op_type.ops) +
                                '\t' + gdb_res.buggy_type + '\n')
        f_summary.close()
