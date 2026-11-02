# VoidSentinel 0.2

Natív Windows 11 x64 alkalmazás: C++20, szabványos Win32 vezérlők, ETW. Nincs Electron, .NET vagy külső futtatókörnyezet.

## Fordítás

Visual Studio Community: Desktop development with C++, MSVC x64/x86, Windows 11 SDK, C++ CMake tools for Windows.
Futtasd a `build.ps1` fájlt. Új EXE: `out\Release\VoidSentinel.exe`. A korábbi `build\Release` mappában az előző változat maradhat.

Az EXE, a címsor és a tálca saját ikonja az `assets/icons/VoidSentinel.windows.ico` erőforrásból töltődik. Méretek: 16, 20, 24, 32, 40, 48, 64, 96, 128 és 256 pixel, átlátszósággal. Az eredeti ikonok az `assets/icons` mappában maradnak. PNG-csere után a `python assets/icons/build_ico.py` újracsomagolja az ICO-t, átméretezés nélkül. A normál C++ fordításhoz Python nem kell.

## Használat

1. Indítsd az EXE-t, és fogadd el a rendszergazdai engedélykérést.
2. **+ Browse...**: célalkalmazások hozzáadása; **Remove**: eltávolítás.
3. **Include child processes**: gyermekfolyamatok követése.
4. **Start**: új, üres gyűjtés; már futó és később induló példányok követése.
5. **Stop**: leállítás; az aktuális eredmény látható és menthető marad.
6. **Save result...**: CSV-mentés szabványos Windows mentési ablakkal. Futás közben az aktuális pillanatkép menthető.

## Összevonás és sorrend

Egy sor kulcsa a teljes alkalmazásútvonal, teljes fájl/könyvtárútvonal és művelet. Kis-/nagybetűtől független Windows ordinal összehasonlítás. Tíz írás ugyanarra a fájlra egy Write sor, Count = 10 értékkel. Más művelet vagy más alkalmazás külön sor. Ugyanazon EXE több példánya közös eredményt ad.

Alkalmazások EXE-nevük szerint ABC-sorrendben, azonos névnél teljes alkalmazásútvonal szerint. Alkalmazásonként külön fejlécsor. Alkalmazáson belül könyvtárak elöl, majd fájlok; mindkét rész teljes útvonal szerint ABC-sorrendben. Azonos útvonalnál művelet szerint rendez.

Az első és utolsó időpont, darabszám és kimenetel megmarad. Eltérő kimeneteleknél Mixed results látható. A CSV külön számlálja a sikeres, sikertelen és nem megerősített kéréseket. A típus az ETW megnyitási opcióiból származik és az ismert típus az útvonal más műveleteire is átkerül. Ismeretlen típus Unknown jelöléssel a csoport végén marad.

Az eredmények kizárólag memóriában élnek. Nincs automatikus adatbázis vagy eredményfájl. Új Start és bezárás elveszíti az el nem mentett gyűjtést. Csak a célalkalmazások listája marad meg: `%LOCALAPPDATA%\VoidSentinel\targets.txt`. Az előző verzió adatbázisait nem tölti be és nem törli; az SQLite már nem része a fordításnak.

A CSV ugyanazokat az összevont sorokat és sorrendet tartalmazza: UTF-8 BOM, vesszős elválasztás, idézőjeles mezők, CRLF sorvégek. Magyar Excelben szükség esetén szöveg/CSV importtal válassz vessző elválasztót. Nincs 10 000 soros listakorlát; a memóriaigény az egyedi eredmények számával nő.

## Korlátok

Létrehozás/felülírás, írás, méret, attribútum/időbélyeg módosítás, átnevezési és törlési kérések. A puszta sikeres megnyitásokat és olvasásokat nem listázza. Success az I/O-kérés sikeres visszatérése; nem bizonyít lemezre tartósulást. A végleges törlést nyitott handle késleltetheti. Átnevezésnél jelenleg az eredeti útvonal látható.

Hiányzó befejezésnél Completion unavailable, függő állapotnál Pending (not confirmed) az eredmény. A fájlnév-cache korlátos; a függő kérések felső határa 50 000. Feloldhatatlan útvonalnál külön fájlobjektum-azonosító látható.

A rendszerszintű fájleseményeket folyamatonként szűri. Más szolgáltatásnak átadott munka a közvetlen végrehajtóhoz tartozik. Memóriatérképezett és késleltetett rendszerírások eredeti alkalmazáshoz rendelése nem garantált. Az állapotsor jelzi az összes eseményt, ETW-veszteséget, eldobásokat, feloldatlan útvonalakat és hiányzó befejezéseket. Egyszerre egy példány figyelhet. Saját árva ETW-munkamenetét a következő Start helyreállítja.

## Tesztek

`ctest --test-dir out -C Release --output-on-failure`

A valódi ETW-teszthez az EXE-t rendszergazdaként indítsd a `--capture-test "FRISS_TESZTMAPPA"` argumentummal. Másik példányban ne fusson figyelés. Ellenőrzés: `python tests\verify_capture.py FRISS_TESZTMAPPA`. A Python csak fejlesztői ellenőrzéshez kell. A tesztsegéd a megadott mappán belül végez fájlműveleteket és kifejezetten exportál CSV-be.

Tesztelés: ékezetes útvonalak, könyvtár- és fájlműveletek, gyermekfolyamatok be-/kizárása, idegen folyamat kizárása, több célalkalmazás, újraindítás, 10 000 írás egy sorba összevonása, CSV-sorrend, vegyes kimenetelek, kis-/nagybetűk, típusöröklés, ürítés.

Forrás: https://learn.microsoft.com/en-us/windows/win32/etw/fileio
