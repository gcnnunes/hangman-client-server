//c program specifying the particular char position from the string
#include<stdio.h>
#include<string.h>
int main()
{
	char *str,c;int i,f,lenf=0;
	printf("\nenter a string:");
	scanf("%s",str);
	lenf=strlen(str);
	printf("\nenter a character to find its position:");
	scanf(" %c",&c);
	for(i=0;i<lenf;i++)
	{
		if(str[i]==c)
		{
			printf("character position:%d\n",i+1);
			f=1;
		}
	}
	if(f==0)
	{
		printf("\ncharacter not found");
	}

	return(0);
}