import os

files = []
filelist = os.listdir(".")
for file in filelist:

	if file.split(".")[-1] == "txt":

		output = []
		with open(file, "r") as data_output:
			with open(file.split(".")[0]+"OUT.txt", "w") as data_input:
				
				for line in data_output:
					if line != "\n" and line.split()[-1][3:7] == "Free" and line.split()[-1] not in output:
							print line.split()[-1]
							output.append(line.split()[-1][3:])

				for linedata in output:
					data_input.write(linedata+"\n")