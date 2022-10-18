
import sys
import random


names = [	"Anna", "Armando", "Alex", "Alessio", 
		    "Beatrice", "Bruno", "Boris", "Barbara",
		    "Cristian", "Clara", "Carlo", "Cassandra",
		    "Debora", "Dante", "Dimitri",
    		"Emanuele", "Erika", "Ester", "Ezio",
	    	"Fabrizio", "Federico", "Francesco", "Filippo",
    		"Giulia", "Gennaro", "Gemma", "Giuseppe",
	    	"Ivan", "Iris", "Ilenia",
    		"John", "Jack", "Julio", "Julian",
	    	"Luna", "Lamberto", "Leonardo", "Lucio",
    		"Margherita", "Maria", "Massimiliano", "Mimmo",
	    	"Natalia", "Nathan", "Noemi", "Nicola",
    		"Pippo", "Pluto", "Paolo", "Piero",
	    	"Roberto", "Rosa", "Remo", "Ramona",
		    "Silvia", "Sara", "Stefano", "Simone",
    		"Valentino", "Veronica", "Vera", "Vinicio" ]


# Generates a string composed of random numbers between 0 and 9 of given length
def GeneratePhoneNumber(length):

	number = ""
	for i in range(0, length):
		number += str(random.randint(0, 9));

	return number;


def GeneratePhonebook():

	if len(sys.argv) != 3:
		print("usage is: " + sys.argv[0] + " <filename> <number of elements>")
		return;

	random.seed();
	outfile = open(sys.argv[1], "w")

	print("Generating...")

	for i in range(0, int(sys.argv[2])):
		currentName = names[ random.randint(0, len(names) - 1) ]
		currentNum = GeneratePhoneNumber(10)

		outfile.write(currentName + ";" + currentNum + "\n")
		

	outfile.close()
	print("Completed!")


GeneratePhonebook();
