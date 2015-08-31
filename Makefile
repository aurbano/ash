all:
	g++ -I src/includes/ src/builtins/* src/utils/* src/ash.cpp -o ash

debug:
	g++ -g -I src/includes/ src/builtins/* src/utils/* src/ash.cpp -o ashd
