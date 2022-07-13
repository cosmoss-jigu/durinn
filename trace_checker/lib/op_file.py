import os
import sys
import subprocess
import random

BUGGY_TYPE_0 = 'no_out_file'
BUGGY_TYPE_1 = 'out_file_len_mismatch'
BUGGY_TYPE_2 = 'out_file_val_mismatch'

MAGIC = '!!!'
MAGIC_ANY = '###'
MAGIC_PREV = '%%%'

class OPFile:
    def __init__(self, op_file):
        self.op_file = op_file
        self.ops = open(op_file).read().split("\n")
        self.ops= self.ops[:-1]

    def gen_prefix(self, pair, output):
        pair_min = min(pair[0], pair[1])
        pair_max = max(pair[0], pair[1])
        f = open(output, 'w')
        for i in range(len(self.ops)):
            if (i == pair_min):
                continue
            if (i == pair_max):
                break
            f.write(self.ops[i] + '\n')

    def gen_race(self, pair, output):
        f = open(output, 'w')
        f.write(self.ops[pair[0]] + '\n')
        f.write(self.ops[pair[1]] + '\n')

class PrefixOPFile:
    def __init__(self, op_file):
        self.op_file = op_file
        self.ops = open(op_file).read().split("\n")
        self.ops= self.ops[:-1]

    def gen_model(self):
        model = dict()
        for op in self.ops:
            entries = op.split(';')[:-1]
            op_type = entries[0]
            # insert
            if op_type == 'i':
                op_key = entries[1]
                op_val = entries[2]
                # TODO: overriding for now
                model[op_key] = op_val
            # delete
            elif op_type == 'd':
                op_key = entries[1]
                # TODO: if not exist
                if op_key in model:
                    del model[op_key]
            # get
            elif op_type == 'g':
                pass
            # TODO: others
            else:
                print(op)
                assert(False)
        return model

class RaceOPType:
    def __init__(self, ops):
        self.ops = ops
        self.init_type()

    def init_type(self):
        #first op
        entries = self.ops[0].split(';')[:-1]
        op_type_0 = entries[0]
        op_key_0 = entries[1]
        #second op
        entries = self.ops[1].split(';')[:-1]
        op_type_1 = entries[0]
        op_key_1 = entries[1]

        if op_type_0 == 'i' and op_type_1 == 'g':
            if op_key_0 == op_key_1:
                self.race_type = 'ig_same'
            else:
                self.race_type = 'ig_diff'
            return

        if op_type_0 == 'd' and op_type_1 == 'g':
            if op_key_0 == op_key_1:
                self.race_type = 'dg_same'
            else:
                self.race_type = 'dg_diff'
            return

        if op_type_0 == 'i' and op_type_1 == 'd':
            if op_key_0 == op_key_1:
                self.race_type = 'id_same'
            else:
                self.race_type = 'id_diff'
            return

        if op_type_0 == 'd' and op_type_1 == 'i':
            if op_key_0 == op_key_1:
                self.race_type = 'di_same'
            else:
                self.race_type = 'di_diff'
            return

        if op_type_0 == 'i' and op_type_1 == 'i':
            if op_key_0 == op_key_1:
                self.race_type = 'ii_same'
            else:
                self.race_type = 'ii_diff'
            return

        if op_type_0 == 'd' and op_type_1 == 'd':
            if op_key_0 == op_key_1:
                self.race_type = 'dd_same'
            else:
                self.race_type = 'dd_diff'
            return

        self.race_type = 'other'

class RaceOPFile:
    def __init__(self, op_file, output_file, first_op_done):
        self.op_file = op_file
        self.ops = open(op_file).read().split("\n")
        self.ops= self.ops[:-1]
        assert(len(self.ops) == 2)

        self.output_file = output_file
        self.outputs = open(output_file).read().split("\n")
        self.outputs = self.outputs[:-1]
        assert(len(self.outputs) == 1)

        self.race_op_type = RaceOPType(self.ops)

        self.first_op_done = first_op_done

    def gen_model(self, model):
        # First op
        entries = self.ops[0].split(';')[:-1]
        op_type = entries[0]
        if op_type == 'i':
            op_key = entries[1]
            op_val = entries[2]
            # TODO: overriding for now
            if self.first_op_done:
                model[op_key] = op_val
            else:
                if op_key not in model:
                    model[op_key] = op_val + MAGIC + '(null)'
                else:
                    model[op_key] = op_val + MAGIC + model[op_key]
        # delete
        elif op_type == 'd':
            op_key = entries[1]
            # TODO: if not exist
            if self.first_op_done:
                model[op_key] = '(null)'
            else:
                if op_key not in model:
                    model[op_key] = '(null)'
                else:
                    model[op_key] = '(null)' + MAGIC + model[op_key]

        # Second op
        entries = self.ops[1].split(';')[:-1]
        op_type = entries[0]
        # insert
        if op_type == 'i':
            op_key = entries[1]
            op_val = entries[2]
            # TODO: overriding for now
            model[op_key] = op_val
        # delete
        elif op_type == 'd':
            op_key = entries[1]
            # TODO: if not exist
            model[op_key] = '(null)'
        # get
        elif op_type == 'g':
            # get may return null and we treat it as a value
            op_key = entries[1]
            op_val = self.outputs[0]
            model[op_key] = op_val
        # TODO: others
        else:
            print(entries)
            assert(False)
        return model

class OPValidator:
    def __init__(self, prefix_op_file, race_op_file, race_output_file, need_del, first_op_done):
        self.prefix_op_file = PrefixOPFile(prefix_op_file)
        self.race_op_file = RaceOPFile(race_op_file, race_output_file, first_op_done)
        self.need_del = need_del

    def gen_model(self):
        self.model = self.race_op_file.gen_model(self.prefix_op_file.gen_model())

    def gen_val_input_and_oracle(self):
        val_inputs = []
        val_oracles = []
        for key in self.model:
            val_input = 'g;' + key + ';'
            val_inputs.append(val_input)
            val_oracle = self.model[key]
            val_oracles.append(val_oracle)

        if self.need_del:
            for key in self.model:
                val_input = 'd;' + key + ';'
                val_inputs.append(val_input)
                val_oracle = ''
                val_oracles.append(val_oracle)

            for key in self.model:
                val_input = 'g;' + key + ';'
                val_inputs.append(val_input)
                #TODO
                val_oracle = '(null)'
                val_oracles.append(val_oracle)

        self.val_inputs = val_inputs
        self.val_oracles = val_oracles

    def exec_prog(self, exe, img, pmsize, pmlayout, opfile, outfile):
        self.timeout = False
        with open(opfile, "w") as f:
            for line in self.val_inputs:
                f.write(line + '\n')
        cmd = [exe,
               img,
               pmsize,
               pmlayout,
               opfile,
               outfile,
               '0']
        # os.system(' '.join(cmd));
        my_env = os.environ.copy()
        # my_env['PMEM_IS_PMEM_FORCE'] = '0'

        # dead lock
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, env=my_env)
        try:
            proc.communicate(timeout=16)
        except subprocess.TimeoutExpired as e:
            proc.kill()
            proc.communicate()
            #print('timeout:' + str(self.race_op_file.ops), file=sys.stderr)
            self.timeout = True

    def validate(self, val_out, oracle_out):
        buggy_type = None
        if self.timeout == True:
            return buggy_type

        with open(oracle_out, "w") as f:
            for line in self.val_oracles:
                f.write(line + '\n')
        if not os.path.exists(val_out):
            with open('BUG', "w") as f:
                pass
            buggy_type = BUGGY_TYPE_0
            return buggy_type

        outputs = open(val_out).read().split("\n")[:-1]
        if len(outputs) != len(self.val_oracles):
            with open('BUG', "w") as f:
                pass
            buggy_type = BUGGY_TYPE_1
        else:
            prev = None
            for i in range(len(self.val_oracles)):
                output = outputs[i]
                oracle = self.val_oracles[i]
                if oracle == '':
                    continue
                if not self.match(output, oracle, prev):
                    with open('BUG', "w") as f:
                        pass
                    buggy_type = BUGGY_TYPE_2
                    break
                prev = output
        return buggy_type

    def match(self, output, oracle, prev):
        if MAGIC_ANY in oracle:
            return True

        if MAGIC_PREV in oracle:
            return output.strip() == prev

        if MAGIC not in oracle:
            return output == oracle

        for o in oracle.split(MAGIC):
            if output == o:
                return True
        return False

class LSOPValidator(OPValidator):
    def __init__(self, op_file, ls_op_idx, need_del):
        self.op_file = op_file
        self.ls_op_idx = ls_op_idx
        self.need_del = need_del

    def gen_model(self):
        self.ops= self.op_file.ops
        model = dict()
        idx = 0
        for op in self.ops:
            if idx > self.ls_op_idx:
                break
            entries = op.split(';')[:-1]
            op_type = entries[0]
            # insert
            if op_type == 'i':
                op_key = entries[1]
                op_val = entries[2]
                # TODO: overriding for now
                if idx != self.ls_op_idx:
                    model[op_key] = op_val
                else:
                    if op_key not in model:
                        model[op_key] = op_val + MAGIC + '(null)'
                    else:
                        model[op_key] = op_val + MAGIC + model[op_key]

            # delete
            elif op_type == 'd':
                op_key = entries[1]
                # TODO: if not exist
                if idx != self.ls_op_idx:
                    if op_key in model:
                        model[op_key] = '(null)'
                else:
                    if op_key not in model:
                        model[op_key] = '(null)'
                    else:
                        model[op_key] = '(null)' + MAGIC + model[op_key]
            # get
            elif op_type == 'g':
                pass
            # TODO: others
            else:
                print(op)
                assert(False)
            idx += 1
        self.model = model

    def gen_val_input_and_oracle(self):
        val_inputs = []
        val_oracles = []



        extra_keys = dict()
        for i in range(100):
            key = str(random.randint(1, 1000))
            while key in extra_keys or key in self.model:
                key = str(random.randint(0, 1000))
            extra_keys[key] = key

        for key in extra_keys:
            val_input = 'i;' + key + ';' + key + ';'
            val_inputs.append(val_input)
            val_oracle = ''
            val_oracles.append(val_oracle)

        for key in extra_keys:
            val_input = 'g;' + key + ';'
            val_inputs.append(val_input)
            val_oracle = extra_keys[key]
            val_oracles.append(val_oracle)

        for key in self.model:
            val_input = 'g;' + key + ';'
            val_inputs.append(val_input)
            val_oracle = self.model[key]
            val_oracles.append(val_oracle)

        #for key in self.model:
        #    val_oracle = self.model[key]
        #    if MAGIC not in val_oracle:
        #        continue

        #    val_input = 'g;' + key + ';'
        #    val_inputs.append(val_input)
        #    val_oracles.append(val_oracle)

        #    val_input = 'r;' + key + ';' + str(int(key)+2) + ';'
        #    val_inputs.append(val_input)
        #    val_oracles.append(MAGIC_PREV)

        if self.need_del:
            for key in self.model:
                val_input = 'd;' + key + ';'
                val_inputs.append(val_input)
                val_oracle = ''
                val_oracles.append(val_oracle)

            for key in self.model:
                val_input = 'g;' + key + ';'
                val_inputs.append(val_input)
                #TODO
                val_oracle = '(null)'
                val_oracles.append(val_oracle)

        self.val_inputs = val_inputs
        self.val_oracles = val_oracles
