all:
	g++ -o a a.cpp -g -fsanitize=address -fsanitize=undefined -Winline

clean:
	rm -rf a
