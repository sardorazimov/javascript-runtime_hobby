C++ Offline Quick Reference — macOS

İnternet olmadan C++ geliştirmek için gerekli temel komutlar.

1. Compiler kontrolü
clang++ --version

Beklenen:

Apple clang version ...
Target: arm64-apple-darwin...

1. C++ dosyasını derleme

Örneğin:

main.cpp

Derle (yalnızca V8 gibi harici bağımlılığı olmayan tek dosyalı örnekler için):

clang++ main.cpp -o main

Bu komut MiniNode projesi için yeterli değildir. MiniNode'u aşağıdaki CMake
komutlarıyla derleyin; V8 başlık ve kütüphanelerini otomatik ekler.

1. Programı çalıştırma
./main

2. Tek komutta Build + Run
clang++ main.cpp -o main && ./main

Eğer compilation başarısız olursa ./main çalıştırılmaz.

1. C++ standardı belirtmek

C++17:

clang++ -std=c++17 main.cpp -o main

C++20:

clang++ -std=c++20 main.cpp -o main

C++23:

clang++ -std=c++23 main.cpp -o main

1. Warning'leri açarak derleme
clang++ -std=c++20 -Wall -Wextra -pedantic main.cpp -o main

Build + Run:

clang++ -std=c++20 -Wall -Wextra -pedantic main.cpp -o main && ./main

1. Debug build
clang++ -std=c++20 -Wall -Wextra -g main.cpp -o main

2. Birden fazla .cpp dosyası

Örneğin:

main.cpp
Player.cpp
Game.cpp

Derleme:

clang++ -std=c++20 main.cpp Player.cpp Game.cpp -o game

Çalıştırma:

./game

1. Tüm .cpp dosyalarını birlikte derleme
clang++ -std=c++20 *.cpp -o app

Çalıştırma:

./app

 1. VS Code'da terminal açma
Terminal → New Terminal

Kısayol:

Control + `

 1. Klasördeki dosyaları görme
ls

Bulunduğun klasörü görme:

pwd

Klasöre girme:

cd klasor_adi

Bir üst klasöre çıkma:

cd ..

 1. Temizleme

Oluşturduğun executable'ı silmek:

rm main

Örneğin:

rm game

 1. Git — internet olmadan

Değişiklikleri kontrol et:

git status

Dosyaları ekle:

git add .

Commit:

git commit -m "update"

git add ve git commit için internet gerekmez.

 1. Git — internet gerektiren komutlar

Push:

git push

Pull:

git pull

Clone:

git clone <repository>

git push, git pull ve git clone internet bağlantısı gerektirir.

 1. Acil durum — en önemli komut

Sadece şunu hatırla:

clang++ -std=c++20 main.cpp -o main && ./main

Bu komut:

main.cpp dosyasını C++20 ile derler.

main isimli executable oluşturur.

Derleme başarılıysa programı çalıştırır.

 1. Birden fazla dosyada acil durum
clang++ -std=c++20 *.cpp -o app && ./app

 2. Compiler nerede?
which clang++

Mac'teki Apple Clang bilgisi:

clang++ --version

Mevcut sistemde compiler:

Apple clang version 21.0.0
Target: arm64-apple-darwin25.6.0

⚡ 30 Saniyelik Hatırlatma
Tek dosya
clang++ -std=c++20 main.cpp -o main && ./main

Birden fazla dosya
clang++ -std=c++20 *.cpp -o app && ./app

Compiler kontrol
clang++ --version

## MiniNode Öğrenme Runtime'ı

Kök dizindeki C++ projesi V8 JavaScript motorunu gömer. `src/` altındaki Rust
projesi ayrıdır ve bu CMake hedefine dahil değildir.

### Gereksinimler

- macOS, Xcode Command Line Tools ve CMake
- Homebrew ile V8 SDK: `brew install v8`

### Derleme ve çalıştırma

VS Code'da CMake kit olarak AppleClang'ı seçin, projeyi yapılandırıp derleyin.
Terminalden kullanmak için:

```sh
cmake -S . -B build -DMININODE_V8_ROOT="$(brew --prefix v8)"
cmake --build build --target mininode
./build/mininode app.js
```

Dosya verilmezse runtime varsayılan olarak `app.js` dosyasını çalıştırır.

### İlk sürümde bulunan API'ler

- `console.log`, `console.info`, `console.warn`, `console.error`
- `require()` ile yerel `.js` dosyaları ve `fs`, `buffer`, `net` built-in'leri
- `fs.readFile`, `fs.readFileSync`, `fs.writeFileSync`, `fs.existsSync`
- `Buffer.alloc`, `Buffer.from`, `Buffer.isBuffer` ve UTF-8 `toString()`
- `setTimeout`, `clearTimeout` ve Promise mikro-görev kontrol noktaları
- `net.tcpRequestSync(host, port, request)` ile temel TCP deneyi

`app.js` bu API'leri birlikte dener ve `mini-node-output.txt` dosyasını
oluşturur. TCP isteği senkrondur; TLS, HTTP sunucusu, npm paketleri, ES module
sözdizimi ve Node'un tüm `Buffer` davranışları bu eğitim projesinin kapsamı
dışındadır.

Git offline
git add .
git commit -m "update"

Git online
git push
git pull

İnternet yoksa: C++ yaz → clang++ ile build et → ./main ile çalıştır → git commit yap.

İnternet gelince: git push yap.
