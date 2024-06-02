#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define BSIZE 512

char buf[BSIZE];

void _error(const char *msg) {
	printf(1, msg);
	printf(1, "ssufs_test failed...\n");
	exit();
}

void _success() {
	printf(1, "ok\n");
}

void test(int ntest, int blocks) {
	char filename[16] = "file";
	int fd, i, ret = 0;

	filename[4] = (ntest % 10) + '0';

	printf(1, "### test%d start\n", ntest);
	printf(1, "create and write %d blocks...\t", blocks);
	fd = open(filename, O_CREATE | O_WRONLY);
	
	if (fd < 0)
		_error("File open error\n");
	 
	for (i = 0; i < blocks; i++) {
		ret = write(fd, buf, BSIZE);
		if (ret < 0) break;
	}
	if (ret < 0)
		_error("File write error\n");
	else
		_success();

	printf(1, "close file descriptor...\t");
	
	if (close(fd) < 0)
		_error("File close error\n");
	else
		_success();

	printf(1, "open and read file...\t\t");
	fd = open(filename, O_RDONLY);
	
	if (fd < 0)
		_error("File open error\n");

	for (i = 0; i < blocks; i++) {
		ret = read(fd, buf, BSIZE);
		if (ret < 0) break;
	}
	if (ret < 0)
		_error("File read error\n");
	
	if (close(fd) < 0)
		_error("File close error\n");
	else
		_success();

	printf(1, "unlink %s...\t\t\t", filename);

	if (unlink(filename) < 0)
		_error("File unlink error\n");
	else
		_success();

	printf(1, "open %s again...\t\t", filename);
	fd = open(filename, O_RDONLY);
	
	if (fd < 0) 
		printf(1, "failed\n");
	else
		printf(1, "this statement cannot be runned\n");

	printf(1, "### test%d passed...\n\n", ntest);
}

int main(int argc, char **argv)
{
	for (int i = 0 ; i < BSIZE; i++) {
		buf[i] = BSIZE % 10;
	}

	test(1, 5);
	test(2, 500);
	test(3, 5000);
	test(4, 50000);	
	exit();
}
