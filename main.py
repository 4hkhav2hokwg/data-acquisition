import json
import time
import subprocess
import numpy as np
from drawnow import *
import matplotlib.pyplot as plt


def draw_plot():
		plt.grid(True)

		plt.plot(data)


def main():
	print("FUCK YOU")

	global data
	data = []
	counter = 0
	nbrSamples = 100
	# data = np.zeros(15)

	process = subprocess.Popen(r"C:\Users\SE1\Desktop\final_fucking_test\main.exe",
								stdout=subprocess.PIPE, encoding='utf-8')

	while True:
			output = process.stdout.readline()

			if output == '' and process.poll() is not None:
				break

			if output:
				# data[counter] = output
				# counter += 1
				data.append(output)

			if len(data) == nbrSamples:
				drawnow(draw_plot)
				data = []

	print(data)


if __name__ == '__main__':
	main()
