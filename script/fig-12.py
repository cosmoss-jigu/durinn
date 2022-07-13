#!/usr/bin/python3

import os
import sys
import matplotlib.pyplot as plt

tasks = {
    "CCEH"           : os.getenv("DURINN_HOME") + "/benchmark/CCEH/random/1000",
    "FAST_FAIR"      : os.getenv("DURINN_HOME") + "/benchmark/FAST_FAIR/random/1000",
    "P-ART"          : os.getenv("DURINN_HOME") + "/benchmark/RECIPE/P-ART/random/1000",
    "P-CLHT"         : os.getenv("DURINN_HOME") + "/benchmark/RECIPE/P-CLHT/random/1000",
}


def run(app_name, path):
    cur_dir = os.getcwd()

    print(app_name + " starts...")

    os.chdir(path)
    os.system('make clean')
    os.system('make main.trace >/dev/null 2>/dev/null')

    os.system('make count >/dev/null 2>/dev/null')

    os.chdir(cur_dir)

    res(app_name, path)

    print(app_name + " ends.")

    os.system('rm -f /tmp/main.exe* /tmp/opt-*')

def res(app_name, path):
    f_in_path = path + '/durinn.csv'
    f_out_path = 'test-space/' + app_name + '/durinn.dat'

    f_in = open(f_in_path, 'r')
    f_out = open(f_out_path, 'w')

    idx = 0
    for line in f_in.read().split('\n')[:-1]:
        if idx == 0:
            idx += 1
            continue
        if idx > 1000:
            break
        num = line.split(',')[-1]
        f_out.write(str(idx) + ' ' + num + '\n')
        idx += 1

    f_in.close()
    f_out.close()

def plot():
	dname = 'test-space'

	def draw_figure(axs, name, tittle, legend, ylim=1000):

		w_file = dname + '/' + name + '/witcher.dat'
		c_file = dname + '/' + name + '/durinn.dat'
		y_file = dname + '/' + name + '/yat.dat'

		def get_y(file_name):
			l = []
			f = open(file_name, 'r')
			for line in f.readlines():
				res = int(line.split(" ")[1])
				if res > sys.maxsize:
					res = sys.maxsize
				l.append(res)
			f.close()
			return l

		witcher = get_y(w_file)
		ciri = get_y(c_file)
		yat = get_y(y_file)
		axs.plot(range(1000), ciri, 'b-', label='Durinn', linewidth=2)
		axs.plot(range(1000), witcher, 'g--', label='Witcher', linewidth=2)
		axs.plot(range(1000), yat, 'r-', label='Yat', linewidth=2)
		axs.set_xlim([-25,1000])
		axs.set_ylim([0,ylim])
		axs.grid(color = 'gray', linestyle = '--', linewidth = 0.5)
		axs.set_title(tittle)
		plt.sca(axs)
		plt.xticks([0, 250, 500, 750, 1000])
		plt.yticks([2500, 5000, 7500, 10000])
		plt.xlabel("# ops")
		plt.ylabel("# tests")
		if legend:
			plt.legend(loc='best')

	fig, axs = plt.subplots(2, 2)

	draw_figure(axs[0][0], 'CCEH', 'CCEH', True)
	draw_figure(axs[0][1], 'FAST_FAIR', 'Fast-Fair', False)
	draw_figure(axs[1][0], 'P-ART', 'P-ART', False)
	draw_figure(axs[1][1], 'P-CLHT', 'P-CLHT', False)

	fig.tight_layout()
	plt.savefig('fig-12.pdf')

def main():
    for name, path in tasks.items():
        run(name, path)
    plot()

if __name__ == "__main__":
    main()
