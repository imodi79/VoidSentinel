# Műszaki háttér

[Kézikönyv főoldala](README.md) · [English](../en/technical-reference.md)

Ez az oldal a pontosság részleteit és a fejlesztői parancsokat tartalmazza. A mindennapi használathoz a [Gyors kezdés](quick-start.md) oldallal érdemes kezdeni.

## A megfigyelés működése

A VoidSentinel az Event Tracing for Windows (ETW) szolgáltatásával figyeli a fájl-, szál- és folyamateseményeket. Az ETW-események fogadása külön szálon fut.

Egy másik háttérszál kezeli a megfigyelés indítását és leállítását, az eredmények pillanatképeit, az összefoglalókat, a szabályillesztést, a rendezést, az eredményexportot és az alkalmazáslista mentését. A beállítások betöltése és a szabályfájlok importálása/exportálása is háttérfeladat. A felület változtathatatlan pillanatképeket olvas egy virtuális Windows-listavezérlőn keresztül; a nagy, leváltott nézeteket a háttérszál szabadítja fel.

A szabályminták feloldása nézetenként egyszer történik. Az átnevezési kapcsolatokat alkalmazás és útvonal szerint indexeli a program. A kérések a memóriában összevonva tárolódnak, ezért egy meglévő bejegyzés ismételt tevékenysége a számlálókat növeli, nem minden kéréshez új sort tárol.

## A kérések körültekintő értelmezése

- A **Success** a megfigyelt Windows-kérés sikeres visszatérését jelenti. Gyorsítótárazott vagy memóriába leképezett írások később, akár másik folyamaton keresztül is eljuthatnak az adattárolóra.
- A **Deletion requested** elfogadott törlési kérést rögzít. Ha valamelyik program még nyitva tartja a fájlt, a tényleges eltávolítás késhet.
- A **Transient candidate** azt jelenti, hogy létrehozást és azt követő elfogadott törlési kérést figyeltünk meg. Nem igazolja, hogy az útvonal jelenleg nem létezik.
- A folyamatláncok megfigyelt szülő-gyermek kapcsolatokat írnak le, nem bizonyítják a felhasználó szándékát. Független szolgáltatásnak átadott munka ahhoz a szolgáltatáshoz tartozik, ha azt külön figyeljük.
- Csak a Start utáni tevékenység áll rendelkezésre. A korábbi műveletek nem rekonstruálhatók.
- Az átnevezési összefoglalók a kernel fájleseményei és a kérésazonosító alapján párosított sikeres célútvonalakat használják. A nem egyértelmű, átfedő életutak külön maradnak. A hiányos párosítások a technikai és nem megerősített nézetekben vizsgálhatók.
- Mappaátnevezések, merev hivatkozások, útvonalálnevek és átfedő életutak hiányossá tehetik az útvonalalapú összefoglalókat.
- Az egyszerű nézet egy olyan bejegyzés sikeres kéréseit is megjelenítheti, amely belsőleg sikertelen kéréseket is tartalmaz. A többi kimenetel a kategóriakapcsolókkal elérhető marad.

## Erőforrásigény és teljesség

A memóriaigény a különböző rögzített bejegyzések számával nő. A függő kérések táblája legfeljebb 50 000 elemet tartalmazhat, és a fájlnév-gyorsítótár mérete is korlátos. A program jelzi a túlcsordulást és az ETW-eseményvesztést.

A Windows fájleseményeit rendszerszinten gyűjtjük, és feldolgozás közben folyamatonként szűrjük. Az erőforrásigény ezért a gép teljes lemeztevékenységétől is függ. A láthatósági szűrő nem szűkíti a Windows eseményforrását.

Egyszerre egy példány rögzíthet. A szabályos bezárás lezárja az ETW-munkamenetet; kényszerített kilépés után egy későbbi Start helyreállíthatja a saját hátramaradt munkamenetét.

## Beállítások és exportformátumok

A beállítások a `%LOCALAPPDATA%\VoidSentinel\targets.txt` és `rules.tsv` fájlokban találhatók. A CSV UTF-8 kódolást, BOM-jelölőt, vesszős elválasztást, idézőjeles szövegmezőket és CRLF rekordlezárásokat használ. Az export és a szabálymentés ideiglenes fájlon keresztül, a célfájl lecserélésével történik.

A szabályok verziózott UTF-8 TSV formátumúak. Az első sor tartalma: `VoidSentinel-Rules`, egy tabulátor, majd `1`. Minden további rekord hat tabulátorral elválasztott mezőt tartalmaz: engedélyezés (`0` vagy `1`), művelet, illesztési mód, útvonal/minta, opcionális alkalmazásminta és leírás. Az üres mezők elválasztó tabulátorai is szükségesek.

A műveletek: `include`, `exclude`, `background`. Az illesztési módok: `folder`, `exact`, `wildcard`. A korlát 4 MiB és 10 000 szabály. Érvénytelen import nem változtatja meg a meglévő listát. A szokásos szerkesztéshez használd a szabályszerkesztőt.

## Tesztek futtatása

[Fordítás](build.md) után a projektmappából futtasd:

```powershell
ctest --test-dir out -C Release --output-on-failure
python tests\verify_csv.py out\results-test.csv
python tests\verify_icons.py dist\VoidSentinel.exe
```

Az ellenőrzőszkriptekhez Python szükséges. A C++ tesztek az összevonást, az összefoglalókat, a szabályokat, a mentést, az érvénytelen importot, a háttérexportot, a párhuzamos pillanatkép-olvasást és a különböző átnevezési célokat vizsgálják.

Valódi ETW-integrációs teszthez állítsd le a többi példányban futó rögzítést, és válassz új tesztmappát. A tesztprogram ezen belül fájlokat hoz létre, ír, nevez át és töröl, majd kifejezetten exportálja az eredményeket:

```powershell
$p = Start-Process .\dist\VoidSentinel.exe -Verb RunAs -ArgumentList '--capture-test "C:\VoidSentinelTest"' -PassThru -Wait
$p.ExitCode
python tests\verify_capture.py C:\VoidSentinelTest
```

Az integrációs tesztek Unicode-útvonalakat, szülő-gyermek tevékenységet, idegen folyamatok kizárását, 10 000 ismételt írást, ideiglenes fájlon keresztüli mentést, elrejtett adatok visszanyerését és a háttérben futó munkamenetkezelőt ellenőrzik.

## Elkülönített felületi tesztek

Ez a fejlesztői cél normál felhasználói jogosultsággal és saját beállításmappával fut, így a felület az emelt jogosultságú megfigyeléstől külön tesztelhető:

```powershell
cmake --build out --config Release --target VoidSentinelUiTest
.\out\ui-test\Release\VoidSentinel.exe
```

A szokásos megfigyeléshez a `dist\VoidSentinel.exe` fájlt használd.

## Ikonfájlok

A beépített ikon 16, 20, 24, 32, 40, 48, 64, 96, 128 és 256 pixeles, átlátszó képeket tartalmaz. A források az `assets/icons` mappában találhatók. PNG-csere után a `python assets/icons/build_ico.py` paranccsal csomagolhatod újra az ICO-t, átméretezés nélkül.

## Windows-dokumentáció

- [Fájlműveleti eseményosztályok](https://learn.microsoft.com/en-us/windows/win32/etw/fileio)
- [ETW-munkamenet létrehozása](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew)
- [Eseménykövetés és veszteségszámlálók](https://learn.microsoft.com/en-us/windows/win32/etw/about-event-tracing)
