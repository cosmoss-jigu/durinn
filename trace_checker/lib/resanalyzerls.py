import os
import shutil
import pickle

class ResAnalyzerLS:
    def __init__(self, workspace, output, src_mapping):
        self.init_workspace(workspace, output)
        self.src_mapping = src_mapping

        # op_type
        #   op_signature
        #       op_idx
        #           full_str
        self.res_map0 = dict()
        # src_line
        #   full_str
        self.res_map1 = dict()

        self.store_ids = set()

        self.run()
        self.print_res()

    def init_workspace(self, workspace, output):
        self.workspace = workspace
        self.output = output + '/' + workspace
        self.res_file = self.output + '/res.txt'
        if not os.path.isdir(output):
            os.mkdir(output)
        os.mkdir(self.output)
        shutil.copy(workspace+'/res.txt', self.output)

        self.op_file = pickle.load(open(workspace+'/opfile.pickle', 'rb'))
        self.trace = pickle.load(open(workspace+'/trace.pickle', 'rb'))

    def run(self):
        dirs = [d for d in os.listdir(self.workspace) if os.path.isdir(os.path.join(self.workspace, d))]
        for d in dirs:
            res_file = self.workspace + '/' + d + '/BUG'
            if not os.path.isfile(res_file):
                continue

            op_type = None
            op_sig = None
            with open(res_file, 'r') as f:
                lines = f.read().split('\n')
                lines = lines[0:-1]
                for line in lines:
                    entry = line.split(',')

                    op_idx = int(entry[0].split(':')[-1])
                    store_id = int(entry[1].split(':')[-1])
                    store_src_num = int(entry[2].split(':')[-1])
                    crash_info = entry[3].split(':')[-1]
                    buggy_type = entry[4].split(':')[-1]

                    # store_src_line
                    store_src_line = self.src_mapping[store_src_num]
                    # op_type
                    if not op_type:
                        op_type = self.op_file.ops[op_idx][0]
                    # op_sig
                    if not op_sig:
                        op_sig = self.trace.gen_signature_ls(op_idx)
                    # full_str
                    full_str = line + ',' \
                                'op_type:' + op_type + ',' \
                                'src_line:' + store_src_line

                    self.store_ids.add(store_id)

                    if op_type not in self.res_map0:
                        self.res_map0[op_type] = dict()
                    if op_sig not in self.res_map0[op_type]:
                        self.res_map0[op_type][op_sig] = dict()
                    if op_idx not in self.res_map0[op_type][op_sig]:
                        self.res_map0[op_type][op_sig][op_idx] = []
                    self.res_map0[op_type][op_sig][op_idx].append(full_str)

                    if store_src_line not in self.res_map1:
                        self.res_map1[store_src_line] = []
                    self.res_map1[store_src_line].append(full_str)

    def print_res(self):
        with open(self.output + '/res.txt', 'a') as f:
            f.write('total_inconsistent_stores:' + str(len(self.store_ids)) + '\n')

        with open(self.output + '/res_op.txt', 'w') as f:
            for op_type in self.res_map0:
                f.write(op_type + ':\n')
                for op_sig in self.res_map0[op_type]:
                    f.write('\t' + op_sig + ':\n')
                    for op_idx in self.res_map0[op_type][op_sig]:
                        f.write('\t\t' + str(op_idx) + ':\n')
                        for full_str in self.res_map0[op_type][op_sig][op_idx]:
                            f.write('\t\t\t' + full_str + '\n')
                        f.write('\n')
                    f.write('\n')
                f.write('\n')

        with open(self.output + '/res_src.txt', 'w') as f:
            for store_src_line in self.res_map1:
                f.write(store_src_line + ':\n')
                for full_str in self.res_map1[store_src_line]:
                    f.write('\t' + full_str + '\n')
                f.write('\n')
