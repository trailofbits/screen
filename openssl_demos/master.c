#include <time.h>    // time()
#include <screen.h>
#include <stdlib.h>
#include <stdio.h>

int tls1_cbc_remove_padding(){

    srand(time(NULL));
    int randomnumber;
    randomnumber = rand() % 3;
    if (randomnumber == 0)
        return 0;
    else if (randomnumber == 1)
        return 1;
    else
    	return -1;	
}

SCREEN(openssl_one)
int tls1_enc()
{
    int tmpret;
    int numpipes = 1;
    int ret = 0;
    
    // vulnerable code extracted from: https://github.com/openssl/openssl/commit/94777c9c86a2b2ea2726c49d6c8f61078558beba
    for (int ctr = 0; ctr < numpipes; ctr++) {
	tmpret = tls1_cbc_remove_padding();
	if (tmpret == -1) // git commit introduces this branch
	    return -1;
	/*
	 *  0: (in non-constant time) if the record is publicly invalid.
	 *  1: if the padding was valid
	 * -1: otherwise.
	 *
	 * so 1 and -1 must be handled in timing-independent code, not 1 and 0
	 */
	ret &= tmpret;
    }

    return ret;
}

int main(){
	printf("[!] return value 0 can be handled in non-constant time\n");
	int d = tls1_enc();
	if (d==0)
		printf("0: padding failure, record publically available\n");
	else if(d==1)
		printf("1: padding was valid\n");
	else
		printf("-1: cbc padding removal failure, record not publically available\n");
	return d;
}
