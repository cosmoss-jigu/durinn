from lib.op_file import OPValidator

class PrefixOPFileQueue:
    def __init__(self, op_file):
        self.op_file = op_file
        self.ops = open(op_file).read().split("\n")
        self.ops= self.ops[:-1]

    def gen_model(self):
        model = []
        for op in self.ops:
            entries = op.split(';')[:-1]
            op_type = entries[0]
            # enq
            if op_type == 'E':
                op_val = entries[1]
                # TODO: overriding for now
                model.append(op_val)
            # deq
            elif op_type == 'D':
                if len(model) > 0:
                    model = model[1:]
            # TODO: others
            else:
                print(op)
                assert(False)
        return model

class RaceOPTypeQueue:
    def __init__(self, ops):
        self.ops = ops
        self.init_type()

    def init_type(self):
        #first op
        entries = self.ops[0].split(';')[:-1]
        op_type_0 = entries[0]
        #second op
        entries = self.ops[1].split(';')[:-1]
        op_type_1 = entries[0]

        self.race_type = op_type_0 + op_type_1

class RaceOPFileQueue:
    def __init__(self, op_file, output_file, first_op_done):
        self.op_file = op_file
        self.ops = open(op_file).read().split("\n")
        self.ops= self.ops[:-1]
        assert(len(self.ops) == 2)

        self.output_file = output_file
        self.outputs = open(output_file).read().split("\n")
        self.outputs = self.outputs[:-1]
        assert(len(self.outputs) == 1)

        self.race_op_type = RaceOPTypeQueue(self.ops)

        self.first_op_done = first_op_done

    def gen_model(self, model):
        ret = []
        # First op
        op_type0 = self.ops[0].split(';')[:-1][0]
        op_type1 = self.ops[1].split(';')[:-1][0]
        if op_type0 == 'E' and op_type1 == 'E':
            ret.append(list(model))
        elif op_type0 == 'D' and op_type1 == 'D':
            model0 = list(model)
            model0 = model0[1:]
            model1 = list(model)
            model1 = model1[2:]
            ret.append(model0)
            ret.append(model1)
        elif op_type0 == 'E' and op_type1 == 'D':
            model0 = list(model)
            model0 = model0[1:]
            ret.append(model0)
        elif op_type0 == 'D' and op_type1 == 'E':
            model0 = list(model)
            model1 = list(model)
            model1 = model1[1:]
            ret.append(model0)
            ret.append(model1)
        else:
            assert(False)
        return ret

class OPValidatorQueue(OPValidator):
    def __init__(self, prefix_op_file, race_op_file, race_output_file, need_del, first_op_done):
        self.prefix_op_file = PrefixOPFileQueue(prefix_op_file)
        self.race_op_file = RaceOPFileQueue(race_op_file, race_output_file, first_op_done)
        self.need_del = need_del

    def gen_model(self):
        self.models = self.race_op_file.gen_model(self.prefix_op_file.gen_model())

    def gen_val_input_and_oracle(self):
        val_inputs = []
        val_oracles = []

        min_l = -1
        for model in self.models:
            if min_l == -1:
                min_l = len(model)
            if len(model) < min_l:
                min_l = len(model)

        for i in range(min_l):
            val_inputs.append('D;')

        for i in range(min_l):
            oracle = ''
            for model in self.models:
                oracle += '!!!' + model[i]
            oracle = oracle[3:]
            val_oracles.append(oracle)


        for i in range(10):
            val_inputs.append('E;999;')
            val_oracles.append('')

        for i in range(2):
            val_inputs.append('D;')
            val_oracles.append('###')

        for i in range(8):
            val_inputs.append('D;')
            val_oracles.append('999')

        self.val_inputs = val_inputs
        self.val_oracles = val_oracles

class LSOPValidatorQueue(OPValidatorQueue):
    def __init__(self, op_file, ls_op_idx, need_del):
        self.op_file = op_file
        self.ls_op_idx = ls_op_idx
        self.need_del = need_del

    def gen_model(self):
        self.models = []
        self.ops= self.op_file.ops
        model = []
        idx = 0
        for op in self.ops:
            if idx > self.ls_op_idx:
                break
            entries = op.split(';')[:-1]
            op_type = entries[0]
            # enq
            if op_type == 'E':
                op_val = entries[1]
                if idx != self.ls_op_idx:
                    model.append(op_val)
                else:
                    self.models.append(model)

            # deq
            elif op_type == 'D':
                if idx != self.ls_op_idx:
                    if len(model) > 0:
                        model = model[1:]
                else:
                    model0 = list(model)
                    model1 = list(model)
                    model1 = model1[1:]
                    self.models.append(model0)
                    self.models.append(model1)
            else:
                print(op)
                assert(False)
            idx += 1
