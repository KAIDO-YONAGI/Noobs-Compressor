#include "heffman.h"


int main(){
    const char *sentence[] = {"mode now ~ ,choose now ~!","OoO.jpg","is NG~","wa ga ma ma ! only input 1 or 2 yo!",
                              "are u want to end this program ? well...I forgot to tell u that has a number to do it.sorry~"};
    srand((unsigned int)time(NULL));

    printf("[kotori] welcome to kotori workshop ! This is a compress program.\n");

    printf("[kotori] sa,let's choose the program mode now~\n");

    printf("[kotori] remember input 1 to compress, else input 2 to decompress yo~~\n");

    printf("[kotori] when ur under a mode, u can input 'end' back to mode selection\n");

    while(1){
        printf("your number to choose the mode : ");
        int mode = 2;
        scanf("%d",&mode);
        getchar();
        if(mode==88){
            printf("[kotori] wow...bye bye yo\n");
            break;
        }
        if(mode==1){
            char_to_code();
        }else if(mode==2){
            code_to_char();
        }else{
            int kotorist = rand() % 5;
            if(kotorist==2){
                printf("[kotori] %d %s\n",mode,sentence[kotorist]);
            }else{
                printf("[kotori] %s\n",sentence[kotorist]);
            }
            continue;
        }
        printf("[kotori] do u want to continue? yes is 1, no is 88\n");
        scanf("%d",&mode);
        getchar();
        if(mode==88) break;
        else if(mode==1) continue;
        else{
            printf("[kotori] oh no!u fuel a boring passion in a such boring thing!  .OoO.jpg\n");
            sleep(1);
            printf("[kotori] ...\n");
            sleep(2);
            printf("[kotori] ja ... let's start again\n");
        }
    }
    printf("[kotori] ma ta ko n do!");
    sleep(3);
    system("pause");
    return 0;
}