import os
import shutil

# ==========================================
# AYARLAR
# ==========================================
GIRDI_DOSYASI = "cutscene.md"
KAYNAK_WEM_KLASORU = os.path.join(".", "wem")
HEDEF_KLASOR = os.path.join(".", "output", "wem_output")

def wem_getir():
    print("--- Cutscene WEM Toplayıcı Başlatılıyor ---\n")

    # 1. Hedef klasörü oluştur (yoksa)
    if not os.path.exists(HEDEF_KLASOR):
        os.makedirs(HEDEF_KLASOR)
        print(f"Hedef klasör oluşturuldu: {HEDEF_KLASOR}")

    # 2. Girdi dosyasını kontrol et
    if not os.path.exists(GIRDI_DOSYASI):
        print(f"HATA: '{GIRDI_DOSYASI}' dosyası bulunamadı!")
        print("Lütfen bu scripti cutscene.md ile aynı klasörde çalıştırın.")
        return

    kopyalanan_sayisi = 0
    bulunamayan_sayisi = 0

    with open(GIRDI_DOSYASI, "r", encoding="utf-8") as f:
        satirlar = f.readlines()

    print(f"'{GIRDI_DOSYASI}' içinden {len(satirlar)} satır taranıyor...\n")

    for satir in satirlar:
        satir = satir.strip()
        if not satir:
            continue
        
        # 3. Sağdan Parçalama (Virgül sorununu çözen kısım)
        # rsplit(',', 2) komutu stringi sağdan sola doğru en fazla 2 kere böler.
        # Örnek Çıktı: ["224010040,'Oh, pardon me. It's...',", "472964748", "Iji"]
        parcalar = satir.rsplit(',', 2)
        
        if len(parcalar) < 3:
            print(f"[ATLANDI] Hatalı Satır Formatı: {satir}")
            continue
            
        # Ortadaki parça bizim WEM ID'miz
        wem_id = parcalar[1].strip()
        
        # ID gerçekten sadece sayılardan mı oluşuyor kontrol edelim (başlık satırlarını falan atlamak için)
        if not wem_id.isdigit():
            continue

        # 4. Kaynak klasörünü belirle (ID'nin ilk 2 hanesi)
        # Örn: "150888717" -> "15"
        klasor_no = wem_id[:2]
        
        kaynak_dosya = os.path.join(KAYNAK_WEM_KLASORU, klasor_no, f"{wem_id}.wem")
        hedef_dosya = os.path.join(HEDEF_KLASOR, f"{wem_id}.wem")

        # 5. Kopyalama işlemi
        if os.path.exists(kaynak_dosya):
            try:
                shutil.copy2(kaynak_dosya, hedef_dosya)
                kopyalanan_sayisi += 1
                # İşlem akışını görmek istersen aşağıdaki satırın başındaki '#' işaretini kaldırabilirsin
                # print(f"[OK] Kopyalandı: {kaynak_dosya}") 
            except Exception as e:
                print(f"[HATA] {wem_id}.wem kopyalanamadı: {e}")
        else:
            print(f"[EKSİK DOSYA] Bulunamadı: {kaynak_dosya}")
            bulunamayan_sayisi += 1

    print("\n" + "="*45)
    print("İşlem Tamamlandı!")
    print(f"Başarıyla Kopyalanan WEM: {kopyalanan_sayisi}")
    if bulunamayan_sayisi > 0:
        print(f"Bulunamayan/Eksik WEM   : {bulunamayan_sayisi}")
    print("="*45)

if __name__ == "__main__":
    wem_getir()