all:
	g++ -I src/includes/ src/builtins/* src/utils/* src/ash.cpp -o ash
