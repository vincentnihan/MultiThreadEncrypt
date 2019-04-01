/*

Family Name: Ni

Given Name: Vincent

Section: E

Student Number: 215120124

CS Login: hyni

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
//#include <ntsid.h>
#include <pthread.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define TEN_MILLIS_IN_NANOS 10000000

typedef  struct {
	char  data;
	off_t offset;
	char state;
} BufferItem;

FILE *input, *output;
BufferItem *buffer;

int curBuffer, maxBuffer, key;

pthread_mutex_t writeLock, BufferItemLock, readLock;

void update()
{
	struct timespec t;
	int seed = 0;
	t.tv_sec = 0;
	t.tv_nsec = rand_r(&seed) % (TEN_MILLIS_IN_NANOS + 1);
	nanosleep(&t, NULL);
}

int findState(char j)
{
	int i = 0;
	for(i = 0; i< maxBuffer; i++)
	{
		if(buffer[i].state == j)
		{

			return i;
		}
	}
	return -1;
}

void *inThread(void *argv)
{
	update();
	char temp;
	while (1)
	{
		int i = 0;
		pthread_mutex_lock(&readLock);
		off_t offset = ((int)ftell(input));
		if ((temp = fgetc(input)) == EOF)
		{
			pthread_mutex_unlock(&readLock);
			break;
		}
		pthread_mutex_unlock(&readLock);
		pthread_mutex_lock(&BufferItemLock);
		while ((i = findState('0')) == -1)
		{
			printf("data in wait since buffer is full \n");
			update();
		}
		printf("data in = %c \n",temp);
		buffer[i].data = temp;
		buffer[i].offset = offset;
		buffer[i].state = '1';
		pthread_mutex_unlock(&BufferItemLock);
		update();
	}
	printf("data in think its finish\n");
	return NULL;
}

void *workThread(void *argv)
{
	update();
	while (1)
	{
		int i = 0;
		pthread_mutex_lock(&BufferItemLock);
		if (feof(input) && findState('1') == -1)
		{
			printf("data work think its finish\n");
			pthread_mutex_unlock(&BufferItemLock);
			break;
		}
		else if ((i = findState('1')) == -1)
		{
			printf("data work wait since buffer is empty \n");
			pthread_mutex_unlock(&BufferItemLock);
			update();
			continue;
		}

		buffer[i].state = '2';
		if (buffer[i].data > 31 && buffer[i].data < 127)
		{
			buffer[i].data = (((int)buffer[i].data - 32) + 2 * 95 + key) % 95 + 32;
		}
		buffer[i].state = '3';
		pthread_mutex_unlock(&BufferItemLock);
		printf("data work = %c \n",buffer[i].data);
		update();

	}
	return NULL;
}

void *outThread(void *argv)
{
	update();
	while(1)
	{
		int i = 0;
		pthread_mutex_lock(&BufferItemLock);
		if (feof(input) && findState('3') == -1 && findState('2') == -1 && findState('1') == -1)
		{
			printf("data out think its finish\n");
			pthread_mutex_unlock(&BufferItemLock);
			break;
		}
		else if((i = findState('3')) == -1)
		{
			printf("data out wait since buffer is empty \n");
			pthread_mutex_unlock(&BufferItemLock);
			update();
			continue;
		}

		off_t offset;
		char data;

		offset = buffer[i].offset;
		data = buffer[i].data;
		printf("data out = %c \n",buffer[i].data);
		buffer[i].state = '0';
		pthread_mutex_unlock(&BufferItemLock);

		pthread_mutex_lock(&writeLock);
		if (fseek(output, offset, SEEK_SET) != EOF)
		{
			fputc(data, output);
		}
		pthread_mutex_unlock(&writeLock);
		update();
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_mutex_init(&writeLock, NULL);
	pthread_mutex_init(&BufferItemLock, NULL);
	pthread_mutex_init(&readLock, NULL);
	printf("argc = %d \n",argc);
	input = fopen(argv[5], "r");
	output = fopen(argv[6], "w");
	key = atoi(argv[1]);
	printf("key = %d \n",key);
	int nIn = atoi(argv[2]);
	printf("nIn = %d \n",nIn);
	int nWork = atoi(argv[3]);
	printf("nWork = %d \n",nWork);
	int nOut = atoi(argv[4]);
	printf("nOut = %d \n",nOut);
	maxBuffer = atoi(argv[7]);
	buffer =  (BufferItem*)calloc(maxBuffer, sizeof(BufferItem));
	int i = 0;
	for(i = 0; i< maxBuffer; i++)
	{
		buffer[i].state = '0';

	}

	pthread_t childThread[nIn + nWork + nOut];
	for (i = 0; i < nIn; i++) {
		pthread_create(&childThread[i], NULL, inThread, NULL);
	}
	for (i = nIn; i < (nIn + nWork); i++) {
		pthread_create(&childThread[i], NULL, workThread, NULL);
	}
	for (i = (nIn + nWork); i < (nIn + nWork + nOut); i++) {
		pthread_create(&childThread[i], NULL, outThread, NULL);
	}

	for (i = 0; i < (nIn + nWork + nOut); i++)
	{
		pthread_join(childThread[i], NULL); //wait for each pthread to end
	}
	printf("All pthread done \n");
	fclose(input);
	fclose(output);
	free(buffer);

	return 0;
}
