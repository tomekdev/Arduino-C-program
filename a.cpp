#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main(int argc, char **argv)
{
	if(argc < 5)
	{
		printf("Za mało argumentów! Sposób użycia:\n%s czytaj ROZMIAR_W_BAJTACH PLIK_WYJŚCIOWY PORT_ARDUINO\n%s wgraj ROZMIAR_W_BAJTACH PLIK_DO_WGRANIA PORT_ARDUINO\n", argv[0], argv[0]);
		return -1;
	}
	int mode = 0;
	if(strcmp(argv[1], "czytaj") == 0)
		mode = 1;
	else if(strcmp(argv[1], "wgraj") == 0)
		mode = 2;
	else
	{
		printf("Nieobsługiwana operacja %s\n", argv[1]);
		return -1;
	}
	int size = strtol(argv[2], NULL, 10);
	if(size < 1)
	{
		printf("Nieprawidłowy rozmiar %d\n", size);
		return -1;
	}
	char *path = argv[3];
	int arduino_port = open(argv[4], O_RDWR);
	if(arduino_port < 0)
	{
		printf("Błąd komunikacji z Arduino przez %s: %s\n", argv[4], strerror(errno));
		return -1;
	}
	
	struct termios t;
	if(tcgetattr(arduino_port, &t))
	{
		printf("Błąd konfiguracji połączenia z Arduino: %s\n", strerror(errno));
		return -1;
	}
#if 0
	t.c_cflag &= ~PARENB;
	t.c_cflag &= ~CSTOPB;
	t.c_cflag &= ~CSIZE;
	t.c_cflag |= CS8;
	t.c_cflag |= CRTSCTS;
	t.c_cflag |= CREAD | CLOCAL;
	t.c_lflag &= ~ICANON;
	t.c_lflag &= ~ECHO;
	t.c_lflag &= ~ECHOE;
	t.c_lflag &= ~ECHONL;
	t.c_lflag &= ~ISIG;
	t.c_iflag &= ~(IXON | IXOFF | IXANY);
	t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	t.c_oflag &= ~OPOST;
	t.c_oflag &= ~ONLCR;
	t.c_oflag = 0;
	t.c_cc[VTIME] = 50;
	t.c_cc[VMIN] = 0;
#endif
	cfmakeraw(&t);
	t.c_cc[VTIME] = 10;
	t.c_cc[VMIN] = 0;
	cfsetspeed(&t, B115200);
	
	if(tcsetattr(arduino_port, TCSANOW, &t) != 0)
	{
		printf("Błąd ustawiania parametrów komunikacji: %s\n", strerror(errno));
		return -1;
	}
	
	if(mode == 1)
	{
		int out = open(argv[3], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(out < 0)
		{
			printf("Błąd zapisu do %s: %s\n", argv[3], strerror(errno));
			return -1;
		}
		int i = -1;
		int bytes_read = 0;
		char cmd[256];
		sprintf(&cmd[0], "r %d", size);
		write(arduino_port, cmd, strlen(cmd));
		printf("DEBUG: wysłano komendę\n");
		while(bytes_read < size)
		{
			char buf[256];
			printf("Begin read\n");
			i = read(arduino_port, &buf, sizeof(buf));
			bytes_read += i;
			write(out, buf, sizeof(char) * i);
			printf("Odczytano %d bajtów\n", bytes_read);
		}
		close(out);
	}
	if(mode == 2)
	{
		int in = open(argv[3], O_RDONLY);
		if(in < 0)
		{
			printf("Błąd odczytu z %s: \n", argv[3], strerror(errno));
			return -1;
		}
		int i = -1;
		int bytes_written = 0;
		char cmd[256];
		sprintf(&cmd[0], "w %d", size);
		write(arduino_port, cmd, strlen(cmd));
		char status[2];
		printf("Oczekiwanie\n");
		read(arduino_port, &status, 1);
		printf("%d\n", status[0]);
		if(status[0] != 1)
		{
			printf("Błąd zapisu\n");
			size = 0;
		}
		while(bytes_written < size)
		{
			status[0] = 0;
			char buf[16];
			i = read(in, &buf, sizeof(buf));
			write(arduino_port, buf, sizeof(char) * i);
			bytes_written += i;
			read(arduino_port, &buf, 16);
//			if(status[0] != 1)
//			{
//				printf("Błąd zapisu\n");
//				break;
//			}
			for(int i = 0; i < 16; i++)
				printf("%c", buf[i]);
			printf("\n");
			printf("Zapisano %d bajtów\n", bytes_written);
		}
		close(in);
		
	}
	close(arduino_port);
return 0;
}
