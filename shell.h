#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>      // printf, perror vb.
#include <stdlib.h>     // exit, malloc vb.
#include <unistd.h>     // fork, exec, pipe vb.
#include <sys/types.h>  // pid_t, size_t vb.
#include <sys/wait.h>   // wait, waitpid
#include <signal.h>     // signal, sigaction
#include <string.h>     // strcmp, strtok vb.
#include <errno.h>      // errno

/*
 * Burada, shell.c içerisinde tanımlanan 
 * fonksiyonlar ve değişkenler için prototipler/extern bildirimleri bulunur.
 * 
 * Not: Tümü global olarak kullanmak zorunda değilseniz,
 * bazılarını statik tanımlayarak sadece shell.c içinde tutabilirsiniz.
 */

/* Maksimum giriş uzunluğu ve kelime sayısı (shell.c içerisinde de kullanılıyor) */
#define MAX_GIRIS_UZUNLUK 1024
#define MAX_KELIME_SAYISI 100

/* 
   Arka plan süreçler için 
   Global değişkenler (shell.c içerisinde tanımlanıyor) 
   Burada extern olarak bildirilebilir
*/
extern pid_t arkaPlanPIDler[100];
extern int arkaPlanIslemSayisi;
extern int quitAktif; // 'quit' komutu aktif mi kontrolü için bayrak

/* Fonksiyon Prototipleri */

/*
 * Boşlukları temizleyen fonksiyon
 * 
 * @param str: İşlenecek string
 * @return: Temizlenmiş string
 */
char* bosluklariTemizle(char *str);

/*
 * Kullanıcıya prompt gösteren fonksiyon
 */
void promptYaz();

/*
 * Noktalı virgül ile ayrılmış komutları bölerek dizi halinde döndüren fonksiyon
 * 
 * @param girdi: İşlenecek komut satırı
 * @param komutSayisi: Komut sayısını döndüren pointer
 * @return: Komutların bulunduğu dizi
 */
char** noktaliVirgulIleBol(char *girdi, int *komutSayisi);

/*
 * Boşluklara göre bir komutu kelimelere ayıran fonksiyon
 * 
 * @param komut: İşlenecek komut stringi
 * @return: Kelimelerin bulunduğu dizi
 */
char** kelimelereBol(char *komut);

/*
 * Pipe (|) ile ayrılmış komutları parçalara ayıran fonksiyon
 * 
 * @param komut: İşlenecek komut stringi
 * @param parcaSayisi: Parça sayısını döndüren pointer
 * @return: Parçaların bulunduğu dizi
 */
char** boruIleBol(char *komut, int *parcaSayisi);

/*
 * Giriş yönlendirme yapan fonksiyon
 * 
 * @param kelimeler: Komutun kelimeler dizisi
 * @return: Başarılıysa 0, aksi halde -1
 */
int girisYonlendirme(char **kelimeler);

/*
 * Çıkış yönlendirme yapan fonksiyon
 * 
 * @param kelimeler: Komutun kelimeler dizisi
 * @return: Başarılıysa 0, aksi halde -1
 */
int cikisYonlendirme(char **kelimeler);

/*
 * Arka plandaki işlemleri sürekli kontrol eden ve tamamlandıklarında bildiren fonksiyon
 */
void arkaPlanIslemleriniKontrolEt();

/* 
 * SIGCHLD sinyali geldiğinde çağrılacak handler fonksiyonu 
 * Arka plandaki süreçlerin tamamlanmasını yakalar ve durumlarını bildirir.
 */
void arkaPlanBitisHandler();

/*
 * Programı sonlandıran fonksiyon
 * Arka plandaki tüm süreçleri bekler ve çıkış yapar.
 */
void programiSonlandir();

/*
 * Birden fazla komutu pipe ile bağlayarak çalıştıran fonksiyon
 * 
 * @param parcalar: Pipe ile ayrılmış komut parçaları
 * @param parcaSayisi: Parça sayısı
 * @param arkaPlanda: Komut arka planda çalışacak mı (1) yoksa ön planda mı (0)
 */
void boruCalistir(char **parcalar, int parcaSayisi, int arkaPlanda);

/*
 * Tek parçalı (tekli) komutları çalıştıran fonksiyon
 * 
 * @param kelimeler: Komutun kelimeler dizisi
 * @param arkaPlanda: Komut arka planda çalışacak mı (1) yoksa ön planda mı (0)
 */
void komutCalistirTekli(char **kelimeler, int arkaPlanda);

/*
 * Herhangi bir komut satırını yorumlayan fonksiyon
 * 
 * @param komutSatiri: Yorumlanacak komut satırı
 */
void komutYorumla(char *komutSatiri);

/*
 * Kabuk uygulamasını başlatan fonksiyon
 */
void kabukCalistir(void);

#endif /* SHELL_H */
