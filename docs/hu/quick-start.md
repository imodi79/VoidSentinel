# Gyors kezdés

[Kézikönyv főoldala](README.md) · [English](../en/quick-start.md)

## 1. Nyisd meg a VoidSentinelt

Nyisd meg a `VoidSentinel.exe` fájlt, és hagyd jóvá a Windows rendszergazdai engedélykérését. Forrásból történő fordítás után a futtatható példány a `dist\VoidSentinel.exe` helyen található.

## 2. Válassz alkalmazást

Kattints a **+ Browse...** gombra, és válaszd ki az alkalmazás `.exe` fájlját. Az EXE az a fájl, amelyet a Windows a program elindításához futtat. Több alkalmazáshoz ismételd meg ezt a lépést.

Ha nem tudod, melyik EXE-t válaszd, nézd meg az alkalmazás parancsikonjának tulajdonságait vagy a telepítési mappáját. Egy kijelölt bejegyzést a **Remove** gombbal vehetsz ki a figyelési listából.

Hagyd bejelölve az **Include child processes** kapcsolót. Ez az alkalmazás által elindított segédprogramokat, például szerkesztőket vagy telepítési segédfolyamatokat is követi. Ha kizárólag a kiválasztott EXE-ket szeretnéd figyelni, a megfigyelés indítása előtt kapcsold ki.

Az EXE hozzáadása kijelöli a programot megfigyelésre. Magát az alkalmazást te nyisd meg vagy kezdd el használni, amikor készen állsz.

## 3. Kezdd az egyszerű nézettel

Hagyd a profilt **Meaningful changes** állásban. Ez összevonja az összetartozó tevékenységeket, és elrejti a gyakori háttérműveleteket.

Kattints a **Start** gombra, majd végezz el egy kisebb feladatot a figyelt alkalmazásban: például ments egy új dokumentumot egy könnyen felismerhető mappába. A már futó alkalmazásokat is figyeli a program.

A megfigyelést a vizsgálandó feladat előtt indítsd el. A **Start** előtti tevékenység nem része a rögzítésnek.

## 4. Állítsd le, és nézd át az eredményt

Kattints a **Stop** gombra, és várd meg a **Stopped** állapotot. Nézd át az eredménylistát. Az alkalmazáscsoportokon belül először a mappák, majd a fájlok láthatók; az útvonalak ábécérendben szerepelnek.

Dupla kattintással további részleteket láthatsz, például a közreműködő programokat és a sikeres, sikertelen vagy nem megerősített kérések számát.

Ha túl kevés vagy túl sok részletet látsz, válts profilt vagy állítsd át a kapcsolókat. A jelenlegi megfigyelés adatai megmaradnak, ezért a nézetet leállítás után is módosíthatod.

## 5. Mentsd el, amire szükséged van

Kattints a **Save result...** gombra, válassz helyet és fájlnevet, majd mentsd el a CSV-fájlt. A táblázatkezelőben történő megnyitásról a [Mentés és megosztás](saving-and-sharing.md) oldalon olvashatsz.

Új megfigyelés indítása vagy az alkalmazás bezárása előtt ments. Minden **Start** új rögzítéssel helyettesíti az előzőt.

**Következő:** [Az eredmények értelmezése →](results.md)
