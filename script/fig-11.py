#!/usr/bin/python3

import os
import numpy as np
import matplotlib.pyplot as plt

tasks = {
    "CCEH"           : os.getenv("DURINN_HOME") + "/benchmark/CCEH/random/1000",
    "FAST_FAIR"      : os.getenv("DURINN_HOME") + "/benchmark/FAST_FAIR/random/1000",
}

GP     = []
PaI    = []
Durinn = []
Manual = []
fontsize = 20

def run(app_name, path):
    cur_dir = os.getcwd()

    print(app_name + " starts...")

    os.chdir(path)
    os.system('make clean')
    os.system('make main.trace >/dev/null 2>/dev/null')

    os.system('make checker-output-ls4 >/dev/null 2>/dev/null')
    os.system('make checker-output-ls3 >/dev/null 2>/dev/null')
    os.system('make checker-output-ls6 >/dev/null 2>/dev/null')
    os.system('make checker-output-ls5 >/dev/null 2>/dev/null')

    res(path)

    print(app_name + " ends.")

    os.system('rm -f /tmp/main.exe* /tmp/opt-*')
    os.chdir(cur_dir)

def res(path):
    def get_lps(n):
        f_res = open(path + '/checker-output-ls' + str(n) + '/res.txt')
        lps = int(f_res.readlines()[2].split(':')[-1])
        return lps
    GP.append(get_lps(4))
    PaI.append(get_lps(3))
    Durinn.append(get_lps(6))
    Manual.append(get_lps(5))

def plot():
	N = 2

	ind = np.arange(N)  # the x locations for the groups
	width = 0.1       # the width of the bars

	br1 = np.arange(len(GP))
	br2 = [x + width*1.8 for x in br1]
	br3 = [x + width*1.8 for x in br2]
	br4 = [x + width*1.8 for x in br3]

	fig, ax = plt.subplots(figsize =(10, 4))
	rects1 = ax.bar(br1, GP,     width, color='black', edgecolor ='black')
	rects2 = ax.bar(br2, PaI,    width, color='gray',  edgecolor ='black')
	rects3 = ax.bar(br3, Durinn, width, color='lightgray', edgecolor ='black')
	rects4 = ax.bar(br4, Manual, width, color='white', edgecolor ='black')

	# add some text for labels, title and axes ticks
	ax.set_ylim([0,14000])
	ax.set_yticks(())
	ax.set_ylabel('Number of LPs', fontsize=fontsize)

	ax.set_xticks(ind + width*1.8*1.5)
	ax.set_xticklabels(('CCEH', 'Fast-Fair'), fontsize = fontsize)

	ax.legend((rects1[0], rects2[0], rects3[0], rects4[0]),
	          ('only Guarded-Protection','only Publish-after-Initialization',
               'Durinn','Manual'),
	          fontsize=fontsize)

	autolabel(ax, rects1)
	autolabel(ax, rects2)
	autolabel(ax, rects3)
	autolabel(ax, rects4)

	fig.tight_layout()
	plt.savefig('fig-11.pdf')

def autolabel(ax, rects):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        if height < 2000:
            h = 1.05 * height
        else:
            h = 1.01 * height
        ax.text(rect.get_x() + rect.get_width()/2., h, '%d' % int(height),
                ha='center', va='bottom', fontsize = fontsize)

def main():
    for name, path in tasks.items():
        run(name, path)
    plot()

if __name__ == "__main__":
    main()
