/******************************************************************************
 * shell.c
 *
 *  İşletim Sistemleri dersi Proje Ödevi için basit bir kabuk (shell) örneği.
 * 
 *  Türkçe yorum satırları ve fonksiyon isimleriyle (increment hariç) 
 *  gösterilmiştir.
 * 
 *  Derleme: 
 *    (Bkz. Makefile)
 * 
 *  Çalıştırma: 
 *    ./shell
 *
 *****************************************************************************/

#include <stdio.h>      // printf, fgets vb.
#include <stdlib.h>     // exit, malloc, free vb.
#include <unistd.h>     // fork, exec, pipe, dup2, chdir vb.
#include <sys/wait.h>   // wait, waitpid vb.
#include <string.h>     // strtok, strcmp vb.
#include <fcntl.h>      // open, O_RDONLY, O_WRONLY vb.
#include <errno.h>      // errno, perror
#include <signal.h>     // sigaction, kill, SIGCHLD, SIGINT vb.
#include "shell.h"      // Shell ile ilgili prototipler

#define MAX_GIRIS_UZUNLUK 1024
#define MAX_KELIME_SAYISI 100



/*
    Arka plan işlemlerini takip etmek için bir dizi ve sayaç
    - arkaPlanPIDler: Arka planda çalışan proseslerin PID'lerini saklar
    - arkaPlanIslemSayisi: Arka planda çalışan proses sayısını takip eder
*/
pid_t arkaPlanPIDler[100];
int arkaPlanIslemSayisi = 0;
int quitAktif = 0;
int pipeAktif = 0;
/* ----------------------------------------------------------------------------
 * Boşlukları (leading ve trailing) temizleyen yardımcı fonksiyon 
 * (sleep 5 & gibi komutların sonunda '&' kalması için önemlidir).
 * ----------------------------------------------------------------------------*/
char* bosluklariTemizle(char *str) {
    // Baş taraftaki boşlukları atla
    while(*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }
    // Son taraftaki boşlukları atla ve string sonunu belirle
    char *son = str + strlen(str) - 1;
    while(son > str && (*son == ' ' || *son == '\t' || *son == '\n')) {
        *son = '\0';
        son--;
    }
    return str;
}

/* ----------------------------------------------------------------------------
 * 1) Prompt (5 Puan)
 * Kullanıcıya komut girişi için prompt gösterir.
 * ----------------------------------------------------------------------------*/
void promptYaz() {
    printf("> "); // Prompt sembolü
    fflush(stdout); // Çıktıyı hemen terminale gönder
}

/* ----------------------------------------------------------------------------
 * Noktalı virgülle (;) ayrılan komutları bölmek için fonksiyon
 * Örnek: "echo 12; sleep 2; echo 13"
 * ----------------------------------------------------------------------------*/
char** noktaliVirgulIleBol(char *girdi, int *komutSayisi) {
    // Komutları depolamak için dinamik olarak bellek ayır
    char **komutlar = malloc(sizeof(char*) * MAX_KELIME_SAYISI);
    memset(komutlar, 0, sizeof(char*) * MAX_KELIME_SAYISI);

    int index = 0;
    char *token = strtok(girdi, ";");// Noktalı virgül ile böl
    while(token != NULL) {
        // Her bir komutu ayrı bir dizi elemanına kopyala
        komutlar[index] = malloc(strlen(token) + 1);
        strcpy(komutlar[index], token);
        index++;
        token = strtok(NULL, ";"); 
    }
    *komutSayisi = index; // Toplam komut sayısını güncelle
    return komutlar;
}

/* ----------------------------------------------------------------------------
 * Boşluklara göre (space, tab) bir komutu kelimelere ayırma
 * Örn: "ls -l /home"
 * ----------------------------------------------------------------------------*/
char** kelimelereBol(char *komut) {
    char **kelimeler = malloc(sizeof(char*) * MAX_KELIME_SAYISI);
    memset(kelimeler, 0, sizeof(char*) * MAX_KELIME_SAYISI);

    int index = 0;
    char *token = strtok(komut, " \t");  // Boşluk ve tab ile böl
    while(token != NULL) {
        kelimeler[index] = token; // Her kelimeyi diziye ata
        index++;
        token = strtok(NULL, " \t");
    }
    kelimeler[index] = NULL;  
    return kelimeler;
}

/* ----------------------------------------------------------------------------
 * Pipe (|) var mı diye bakıp parçalara ayırır.
 * Örn: "echo 12 | increment | grep 13"
 * ----------------------------------------------------------------------------*/
char** boruIleBol(char *komut, int *parcaSayisi) {
    char **parcalar = malloc(sizeof(char*) * MAX_KELIME_SAYISI);
    memset(parcalar, 0, sizeof(char*) * MAX_KELIME_SAYISI);

    int index = 0;
    char *token = strtok(komut, "|"); // Pipe ile böl
    while (token != NULL) {
        parcalar[index] = malloc(strlen(token) + 1);
        strcpy(parcalar[index], token);
        index++;
        token = strtok(NULL, "|");
    }
    *parcaSayisi = index;
    return parcalar;
}

/* ----------------------------------------------------------------------------
 * Giriş yönlendirme (komut < dosya)
 * Komut içinde '<' operatörü kullanılarak dosya girdisi yapılır.
 * ----------------------------------------------------------------------------*/
int girisYonlendirme(char **kelimeler) {
    int i = 0;
    while(kelimeler[i] != NULL) {
        if(strcmp(kelimeler[i], "<") == 0) { // '<' operatörü bulunursa
            if(kelimeler[i+1] == NULL) { // Dosya adı belirtilmemişse hata
                fprintf(stderr, "Giris dosyasi belirtilmemis.\n");
                return -1;
            }
            int fd = open(kelimeler[i+1], O_RDONLY); // Dosyayı oku modunda aç
            if(fd < 0) { // Dosya açılamadıysa hata mesajı
                fprintf(stderr, "%s giris dosyasi bulunamadi.\n", kelimeler[i+1]);
                return -1;
            }
            dup2(fd, STDIN_FILENO); // Dosya deskriptörünü standart girdi olarak ayarla
            close(fd); //  deskriptörünü kapatt

            // "< dosya" kelimelerini diziden çıkaralım
            while(kelimeler[i+2] != NULL) {
                kelimeler[i] = kelimeler[i+2];
                i++;
            }
            kelimeler[i] = NULL; // Diziyi sonlandır
            break;
        }
        i++;
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Çıkış yönlendirme (komut > dosya)
 * Komut içinde '>' operatörü kullanılarak dosya çıktısı yapılır.
 * ----------------------------------------------------------------------------*/
int cikisYonlendirme(char **kelimeler) {
    int i = 0;
    while(kelimeler[i] != NULL) {
        if(strcmp(kelimeler[i], ">") == 0) { // '>' operatörü bulunursa
            if(kelimeler[i+1] == NULL) {
                fprintf(stderr, "Cikis dosyasi belirtilmemis.\n");
                return -1;
            }
            int fd = open(kelimeler[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);// Dosyayı yazma, oluşturma ve üzerine yazma modunda aç
            if(fd < 0) {
                perror("Dosya acilamadi");
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);

            // "> dosya" kelimelerini diziden çıkaralım
            while(kelimeler[i+2] != NULL) {
                kelimeler[i] = kelimeler[i+2];
                i++;
            }
            kelimeler[i] = NULL;
            break;
        }
        i++;
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Arka plan işlemleri sürekli kontrol edilsin ve biten varsa bildirim yapılsın.
 * (sleep 5 & gibi komutların tam 5 saniye sonrasında "bitti" bildirimi vermesi)
 * ----------------------------------------------------------------------------*/
void arkaPlanIslemleriniKontrolEt() {
    for(int i = 0; i < arkaPlanIslemSayisi; i++) {
        if(arkaPlanPIDler[i] > 0) { // Eğer arka planda bir işlem varsa
            int status;
            pid_t sonuc = waitpid(arkaPlanPIDler[i], &status, WNOHANG); // İşlemi kontrol et
            if(sonuc == arkaPlanPIDler[i]) { // İşlem tamamlandıysa
                int exitCode = WEXITSTATUS(status); // Çıkış kodunu al
                printf("[%d] retval: %d\n", arkaPlanPIDler[i], exitCode); // Kullanıcıya bildir
                fflush(stdout);
                arkaPlanPIDler[i] = 0; // PID'yi sıfırla (işlem tamamlandı)
            }
        }
    }
}
/* ----------------------------------------------------------------------------
 * SIGCHLD sinyali geldiğinde çağrılacak handler
 * Arka plandaki süreçlerin tamamlanmasını yakalar ve durumlarını bildirir.
 * ----------------------------------------------------------------------------*/
void arkaPlanBitisHandler() {
    // quit ve pipe aktifse, handler retval basmasın
    if (pipeAktif || quitAktif) {
        return;
    }
    int status;
    pid_t pid;
    // Birden fazla child bitmiş olabilir
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int exitcode = WEXITSTATUS(status);
        printf("[%d] retval: %d\n", pid, exitcode);
        fflush(stdout);
        printf("> ");
        fflush(stdout);

    }
}

/* ----------------------------------------------------------------------------
 * Programı sonlandır (quit komutu)
 * Arka planda çalışanlar varsa bekle, sonra çık.
 * ----------------------------------------------------------------------------*/
void programiSonlandir() {
// quit aktif edilsin
    quitAktif = 1;
    // Arka planda çalışan tüm süreçleri sırayla bekle
    for (int i = 0; i < arkaPlanIslemSayisi; i++) {// Eğer arka planda bir işlem varsa
        if (arkaPlanPIDler[i] > 0) {
            int status;
            waitpid(arkaPlanPIDler[i], &status, 0);  // Bloklayarak bekle
            int exitCode = WEXITSTATUS(status);      // Çıkış kodunu al
            printf("[%d] retval: %d\n", arkaPlanPIDler[i], exitCode);
            fflush(stdout);
        }
    }
    printf("Kabuk sonlandiriliyor...\n");
    exit(0);
}

/* ----------------------------------------------------------------------------
 * Birden fazla komutu pipe ile bağlama
 * "echo 12 | increment | grep 13" gibi
 * ----------------------------------------------------------------------------*/
void boruCalistir(char **parcalar, int parcaSayisi, int arkaPlanda) {
    int girisFD = 0; // İlk komutun girişi (stdin)
    int fd[2];
    pid_t sonPid = 0;
    pipeAktif = 1;

    for(int i = 0; i < parcaSayisi; i++) {
        if(pipe(fd) < 0) {
            perror("pipe hata");
            return;
        }

        pid_t pid = fork(); // Yeni süreç oluştur
        if(pid < 0) {
            perror("fork hata");
            return;
        }
        else if(pid == 0) {  
            // Çocuk
            // Okuma ucunu standart girdi olarak ayarla
            dup2(girisFD, STDIN_FILENO);

            // Son komut değilse, çıktıyı pipe'ın yazma ucuna yönlendir
            if(i < parcaSayisi - 1) {
                dup2(fd[1], STDOUT_FILENO);
            }
            // Gereksiz dosya tanıtıcılarını kapat

            close(fd[0]);
            close(fd[1]);

            // Komutu boşluklara bölelim
            char *parcaTemiz = bosluklariTemizle(parcalar[i]);
            char **kelimeler = kelimelereBol(parcaTemiz);

            // Giriş / Çıkış yönlendirmesi
            girisYonlendirme(kelimeler);
            cikisYonlendirme(kelimeler);

            // Komutu çalıştır
            execvp(kelimeler[0], kelimeler);
            perror("execvp hata");
            exit(EXIT_FAILURE);
        }
        else {
            // Ebeveyn
            if(arkaPlanda) {
                // Arka planda
                arkaPlanPIDler[arkaPlanIslemSayisi++] = pid;
            }
            else {
                // Ön planda bekleme, 
                // ama tüm pipe'lar oluşturulduktan sonra en sonda tam bekleyeceğiz
                close(fd[1]);// Yazma ucuu kapatt
            girisFD = fd[0];// Okuma ucunu sonraki işlem için ayarla
            sonPid = pid;  // Son komutun PID'sini sakla
            }
        }
    }

    // Ön plandaysa, tüm child'ların bitmesini bekleyelim
    if(!arkaPlanda) {
        for(int i = 0; i < parcaSayisi; i++) {
            wait(NULL); // Tüm child süreçlerini bekle
        }
    }
    pipeAktif = 0;
}

/* ----------------------------------------------------------------------------
 * Tek parçalı (tekli) komut icrası 
 * (örn: "ls -l", "sleep 2", "cat file.txt" vb.)
 * ----------------------------------------------------------------------------*/
void komutCalistirTekli(char **kelimeler, int arkaPlanda) {
    pid_t pid = fork();
    if(pid < 0) {
        perror("fork hata");
        return;
    }
    else if(pid == 0) {
        // Çocuk
        // Girdi / Çıkış yönlendirme
        girisYonlendirme(kelimeler);
        cikisYonlendirme(kelimeler);

        // Komutu çalıştır
        execvp(kelimeler[0], kelimeler);
        perror("Komut calistirilamadi");
        exit(EXIT_FAILURE);
    } else {
        // Ebeveyn
        if(arkaPlanda) {
            // Arka plan: beklenmeyecek
            arkaPlanPIDler[arkaPlanIslemSayisi++] = pid;
        } else {
            // Ön plan: bekle
            waitpid(pid, NULL, 0);
        }
    }
}

/* ----------------------------------------------------------------------------
 * Herhangi bir komut satırını yorumlayan fonksiyon:
 *  - "quit" kontrolü
 *  - sonda '&' var mı?
 *  - pipe var mı?
 *  - değilse tekli komut
 * ----------------------------------------------------------------------------*/
void komutYorumla(char *komutSatiri) {
    // Boşlukları temizle
    komutSatiri = bosluklariTemizle(komutSatiri);

    // Boşsa dön
    if(strlen(komutSatiri) == 0) return;

    // "quit" mi?
    if(strcmp(komutSatiri, "quit") == 0) {
        programiSonlandir();
        return;
    }

    // Arka plan mı diye bak
    int arkaPlanda = 0;
    int len = strlen(komutSatiri);
    if(len > 0 && komutSatiri[len - 1] == '&') { // Komutun sonunda '&' varsa
        // Sondaki & kaldır
        komutSatiri[len - 1] = '\0';  // '&' karakterini kaldır
        // tekrar trim
        komutSatiri = bosluklariTemizle(komutSatiri);
        arkaPlanda = 1;
    }

    // Pipe var mı?
    int parcaSayisi = 0;
    char **parcalar = boruIleBol(komutSatiri, &parcaSayisi);

    if(parcaSayisi > 1) {
        // Birden fazla pipe parçası
        boruCalistir(parcalar, parcaSayisi, arkaPlanda); // Piping işlemini gerçekleştir
    } else {
        // Tek parça
        // Kelimelere böl
        char *temizKomut = bosluklariTemizle(parcalar[0]);
        char **kelimeler = kelimelereBol(temizKomut);
        komutCalistirTekli(kelimeler, arkaPlanda); // Tekli komutu çalıştır

    }

    // Bellek temizliği
    for(int i = 0; i < parcaSayisi; i++) {
        free(parcalar[i]); // Her bir parça için ayrılan belleği serbest bırak 
    }
    free(parcalar);// Parçalar dizisini serbest bırak
}
/* ----------------------------------------------------------------------------
 * Kabuk uygulamasını başlatan fonksiyon
 * Komut döngüsü içerir ve kullanıcıdan komut alır.
 * ----------------------------------------------------------------------------*/
void kabukCalistir(void) {
    // SIGCHLD sinyalini handler'a bağlayalım (arka plan bitişini anında yakalamak için)
    signal(SIGCHLD, arkaPlanBitisHandler);

    while (1) {
        char girdiSatiri[MAX_GIRIS_UZUNLUK];

        // Prompt yaz
        promptYaz();

        // Kullanıcıdan giriş al (Ctrl+D gelirse NULL döner)
        if (fgets(girdiSatiri, MAX_GIRIS_UZUNLUK, stdin) == NULL) {
            printf("\n");
            break;
        }

        // Noktalı virgül (;) ile ayrılmış komutlar olabilir
        int komutSayisi = 0;
        char **komutListesi = noktaliVirgulIleBol(girdiSatiri, &komutSayisi);

        for (int i = 0; i < komutSayisi; i++) {
            char *temizKomut = bosluklariTemizle(komutListesi[i]);
            if (strlen(temizKomut) > 0) {
                komutYorumla(temizKomut);
            }
        }

        // Bellek temizliği
        for (int i = 0; i < komutSayisi; i++) {
            free(komutListesi[i]);
        }
        free(komutListesi);
    }

    // Kabuktan çıkarken arka planda çalışan süreçleri de bekleyelim
    for (int i = 0; i < arkaPlanIslemSayisi; i++) {
        if (arkaPlanPIDler[i] > 0) {
            waitpid(arkaPlanPIDler[i], NULL, 0);
        }
    }

    printf("Kabuk sonlandiriliyor...\n");
}

