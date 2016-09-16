extern void print(int x);

int toto(int f)
{
    int i;
    for(i = 0 ; i < 2 ; i++) {
        f = f*3+1; 
	print(f);
    }
    return f;
}
