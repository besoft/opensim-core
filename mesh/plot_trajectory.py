#/usr/bin/python

import sys

import numpy as np
import pylab as pl

if len(sys.argv) == 1 or len(sys.argv) > 3:
    raise Exception("Only 1 or 2 argument allowed.")

data_filepath = sys.argv[1]
if len(sys.argv) == 3:
    include_zero = False
else:
    include_zero = True


data = np.genfromtxt(data_filepath, names=True, delimiter=',')
fig = pl.figure()
num_plots = len(data.dtype.names) - 1
for i in range(num_plots):
    ax = fig.add_subplot(num_plots, 1, i + 1)
    name = data.dtype.names[i + 1]
    if include_zero:
        ax.axhline(0, color='gray', alpha=0.5)
    ax.plot(data['time'], data[name])
    ax.set_title(name)
    if i == num_plots - 1:
        ax.set_xlabel('time')
pl.show()


