#include <stdio.h>
int main()
{
    int m;
    while (scanf("%d", &m) != EOF)
    {
        int s = 0, a;
        for (int i = 0; i < m; i++)
        {
            scanf("%d", &a);
            s += a;
        }
        printf("%d\n",s);
    }
}