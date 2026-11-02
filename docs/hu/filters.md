# Szűrési szabályok

[Kézikönyv főoldala](README.md) · [English](../en/filters.md)

A szabályokkal elrejtheted az ismétlődő háttérműveleteket, miközben a hasznos tevékenység látható marad. A jelenlegi megfigyelés nézetét módosítják; az elrejtett adatok továbbra is elérhetők, ha később másképp döntesz.

## Mappa kizárása

1. Kattints a **Filter rules...** gombra.
2. Az **Action** mezőben válaszd az **Exclude** lehetőséget.
3. A **Match** mezőben válaszd a **Folder and descendants** lehetőséget.
4. Írd be a mappát a **Path / pattern** mezőbe, vagy használd a **Browse...** gombot.
5. Hagyd üresen az **Application** mezőt, ha minden figyelt alkalmazásra alkalmaznád a szabályt. Egy EXE-névvel szűkítheted a hatókörét.
6. A **Description** mezőben opcionálisan megjegyzést adhatsz a szabály későbbi felismeréséhez.
7. Hagyd bejelölve az **Enabled** kapcsolót, majd kattints az **Add** gombra.
8. A **Save rules** gombbal alkalmazd a listát, és mentsd el a következő indításokra.

Például a `%LOCALAPPDATA%\ExampleApp\Cache` egy alkalmazás gyorsítótárát célozza. Ezt külön kezelheted az alkalmazás beállításait tartalmazó mappától.

Gyorsabb megoldás: kattints jobb gombbal egy eredményre, és válaszd az **Exclude this folder...** vagy az **Always include this folder...** lehetőséget. Ellenőrizd a javasolt szabályt a szerkesztőben, majd mentsd a listát.

## Szabály módosítása, kikapcsolása vagy eltávolítása

Jelöld ki a szabályt, módosítsd a mezőit, majd kattints az **Update selected** gombra. Egy mező átírása önmagában még nem módosítja a listában szereplő szabályt.

Ideiglenes kikapcsoláshoz vedd ki az **Enabled** jelölést, válaszd az **Update selected**, majd a **Save rules** gombot. Eltávolításhoz használd a **Remove selected** gombot, majd ments.

A **Cancel** bezárja a szerkesztőt, és megtartja a korábban aktív listát. A **Defaults** a kezdeti háttérszabályokkal helyettesíti a szerkesztő listáját; ezt a **Save rules** gombbal alkalmazhatod.

## A szabály művelete

| Action | Hatás |
|---|---|
| Include | Megjeleníti az illeszkedő útvonalat az Exclude és Background szabályok ellenére. Az illeszkedő átmeneti és metaadat-bejegyzéseket is megtartja. A sikertelen és nem megerősített kimeneteleket továbbra is az Errors és Unconfirmed kapcsolók szabályozzák. |
| Exclude | Mindkét profilban elrejti az illeszkedő útvonalakat. |
| Background | Elrejti az illeszkedő útvonalakat, amíg a Background kapcsoló ki van kapcsolva. |

**Az Include a szabályok sorrendjétől függetlenül elsőbbséget élvez.** Például kizárhatod egy alkalmazás munkamappáját, és Include szabállyal megtarthatsz benne egy hasznos almappát.

## Az illesztés módja

- **Folder and descendants:** a megnevezett mappára és minden alatta található elemre illeszkedik. A `C:\ExampleCache` illeszkedik a `C:\ExampleCache\item.dat` fájlra, a `C:\ExampleCacheOther` viszont külön marad.
- **Exact path:** egyetlen konkrét fájl- vagy mappaútvonalra illeszkedik.
- **Wildcard:** a `*` tetszőleges karaktersorozatot, a `?` egyetlen karaktert helyettesít. A `*` mappaelválasztókon is átnyúlhat, ezért ellenőrizd a túl tág mintákat.

Az illesztés nem különbözteti meg a kis- és nagybetűket, és mindkét perjelváltozatot elfogadja. Teljes vagy környezeti változóval megadott útvonalat használj.

Az opcionális **Application** mező EXE-nevet, például `ExampleApp.exe`, teljes EXE-útvonalat vagy helyettesítő karakteres mintát, például `Example*.exe` fogad el. A figyelt alkalmazásra és a műveletet végrehajtó segédprogramra is illeszkedhet.

## Másik gépen is használható szabályok

A felhasználónevek és meghajtóbetűjelek gépenként eltérhetnek. A Windows környezeti változóival az aktuális felhasználó mappáira hivatkozhatsz:

| Változó | Jelentés |
|---|---|
| `%LOCALAPPDATA%` | Az aktuális felhasználó helyi alkalmazásadat-mappája. |
| `%APPDATA%` | Az aktuális felhasználó barangoló alkalmazásadat-mappája. |
| `%PROGRAMDATA%` | A gép felhasználói között megosztott alkalmazásadatok. |
| `%USERPROFILE%` | Az aktuális felhasználó profilmappája. |

Például a `%LOCALAPPDATA%\ExampleApp\Cache` az importálás helyén, az ottani gépnek megfelelően oldódik fel. Egy konkrét `D:\ExampleCache` útvonal továbbra is attól függ, hogy az adott meghajtó és mappa releváns-e ott. A teljes elérési utat tartalmazó alkalmazásmintákat is szükséges lehet módosítani.

A mappaválasztó és az eredménylista helyi menüje a felismert felhasználói adatmappáknál lehetőség szerint ezeket a változókat használja.

Összefoglaló nézetben a kizárások a látható végleges útvonalat követik. Egy ideiglenes forrásmappa kizárása ezért nem rejti el a másik mappába átnevezéssel elmentett dokumentumot.

## Kezdeti háttérszabályok

A kezdeti lista bizonyos NVIDIA-gyorsítótárakat és alkalmazás-időbélyegeket, Windows-gyorsítótárakat, helyesírási szótárakat és a Jegyzettömb munkamenet-visszaállítási adatait jelöli háttértevékenységnek. Ezeket ugyanúgy módosíthatod vagy kikapcsolhatod, mint bármely más szabályt. A tágabb alkalmazásadat-területek vizsgálhatók maradnak.

## Szabályok exportálása és importálása

Az **Export...** a szerkesztőben aktuálisan szereplő listát menti, az ott már hozzáadott vagy frissített módosításokkal együtt. Másik gépen válaszd az **Import (replace)...** lehetőséget, ellenőrizd a betöltött listát, majd kattints a **Save rules** gombra.

Az importálás lecseréli a szerkesztő listáját, nem egyesíti a listákat. A **Cancel** megtartja a korábban aktív szabályokat. Érvénytelen import esetén a szerkesztő listája változatlan marad. A teljes átviteli folyamatot a [Mentés és megosztás](saving-and-sharing.md) oldal ismerteti.

**Következő:** [Mentés és megosztás →](saving-and-sharing.md)
