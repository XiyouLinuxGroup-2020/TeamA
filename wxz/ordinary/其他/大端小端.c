#include <stdio.h>
int main()
{
	int a=1;
	char* pc=(char*)&a;
	if(*pc==1)
	printf("小端\n");
	else
	printf("大端\n");
	return 0;
}
