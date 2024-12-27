#include <stdio.h>   // printf, scanf
#include <stdlib.h>  // exit, EXIT_FAILURE
#include "increment.h"

/*
    increment fonksiyonu: parametre olarak aldığı sayıyı 1 arttırır.
*/
int increment(int number) {
    return number + 1;
}

/*
    increment uygulaması (standalone):
    Kullanıcıdan bir tamsayı alır, 1 arttırır ve ekrana basar.
*/
int main() {
    int sayi;
    if (scanf("%d", &sayi) != 1) {
        fprintf(stderr, "Hata: Bir sayı okunamadı.\n");
        return EXIT_FAILURE;
    }
    printf("%d\n", increment(sayi));
    return 0;
}
