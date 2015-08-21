all:
	g++ -I src/includes/ src/builtins/* src/ash.cpp -o ash
