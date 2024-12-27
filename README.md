# Shell (Kabuk) Projesi

## Proje Hakkında
Bu proje, **Linux ortamında** çalışan ve **C programlama dili** ile geliştirilmiş bir basit **komut kabuğu (shell)** uygulamasıdır. Kabuk, kullanıcıdan komutları alır, bu komutları işler ve gerekli çıktıyı üretir. Proje, temel Linux kabuğu özelliklerini kapsayacak şekilde tasarlanmıştır ve kullanıcı dostu bir arayüz sağlar.

## Proje İçeriği

Proje şu temel özellikleri içermektedir:

**Komut Yorumlama:** Kullanıcıdan alınan komutları doğru şekilde çalıştırma.
**Boru (Pipe) Desteği:** Birden fazla komutun çıkışını ve girişini bağlama.
**Arka Plan İşlemleri:** Komutları arka planda çalıştırma ve bitiş durumlarını bildirme.
**Giriş/Çıkış Yönlendirme:** Komutların girişini veya çıkışını dosyalara yönlendirme.
**Özel Sinyal İşleme:** SIGCHLD gibi sinyalleri yakalama ve yönetme.
**Program Sonlandırma:** Tüm arka plan işlemlerini bekleyerek temiz bir şekilde sonlandırma.

## Özellikler
### 1. Temel Komut İşleme
Kullanıcıdan bir komut alır ve çalıştırır. Örneğin:
```bash
> ls -l
```

### 2. Giriş ve Çıkış Yönlendirme
Bir komutun girişini veya çıktısını dosyalara yönlendirme yeteneği sağlar. Örneğin:
```bash
> cat file1.txt > file2.txt
```
Bu komut, `file1.txt` dosyasının içeriğini `file2.txt` dosyasına yazar.

### 3. Boru (Pipe) Desteği
Boru operatörü `|` kullanılarak birden fazla komutun çıktısını birbirine bağlama desteği sunar. Örneğin:
```bash
> echo 12 | ./increment | grep 13
```
Bu komut, `echo` çıktısını `increment` ile işler ve ardından `grep` ile filtreler.

### 4. Arka Plan İşlemleri
Bir komut sonuna `&` eklenerek arka planda çalıştırılabilir. Arka plandaki işlemler tamamlandığında kullanıcı bilgilendirilir. Örneğin:
```bash
> sleep 5 &
```
Tamamlandığında:
```bash
[12345] retval: 0
```
şeklinde bir çıktı alırsınız.

### 5. Çoklu Komut İşleme
Noktalı virgül (`;`) kullanılarak birden fazla komut ardışık olarak çalıştırılabilir. Örneğin:
```bash
> ls -l; echo "Bitti!"
```

### 6. Çıkış Komutu
`quit` komutu ile kabuk sonlandırılır. Kabuk sonlanmadan önce arka planda çalışan tüm işlemler tamamlanır ve kullanıcı bilgilendirilir:
```bash
> quit
[12345] retval: 0
Kabuk sonlandiriliyor...
```

## Kullanım
Kabuk çalıştırıldığında `>` ile başlayan bir komut satırı sunar. Kullanıcı bu satıra komut yazarak işlemleri gerçekleştirebilir.

Örnek Komutlar:
- Temel komutlar: `ls`, `cat`, `echo "Merhaba"`
- Yönlendirme: `cat file1.txt > file2.txt`
- Pipe: `ls | grep txt`
- Arka plan: `sleep 10 &`
- Çoklu komut: `echo Merhaba; ls`
- Çıkış: `quit`

## Dosya Yapısı
```
<repo-adı>/
├── Makefile          # Derleme komutlarını içeren dosya
├── Increment         # increment komutu, giriş olarak aldığı sayıların değerlerini bir artırarak çıktıya yazar.
├── shell.c           # Ana kabuk uygulaması
├── shell.h           # Fonksiyon prototipleri ve global değişkenler
├── main.c            # Kabuk uygulamasını başlatan dosya
├── README.md         # Bu dosya (proje açıklamaları)
```


