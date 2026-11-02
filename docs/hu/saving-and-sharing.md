# Mentés és megosztás

[Kézikönyv főoldala](README.md) · [English](../en/saving-and-sharing.md)

Két külön dolgot menthetsz: a megfigyelés **eredményeit**, valamint a látható tevékenységet szabályozó **szabályokat**.

## Megfigyelési eredmények mentése

1. Állítsd le a megfigyelést, ha a teljes befejezett munkamenet eredményét szeretnéd menteni.
2. Válaszd ki az exportban megjelenítendő profilt, kapcsolókat és szabályokat.
3. Kattints a **Save result...** gombra.
4. Adj meg fájlnevet és mappát a Windows mentési ablakában.
5. Nézd meg az állapotsorban a visszaigazolást vagy az esetleges mentési hibát.

A CSV az aktuális szűrt nézetet tartalmazza, a listával megegyező sorrendben: teljes útvonalakkal, darabszámokkal, időtartományokkal, közreműködőkkel, átnevezési célokkal és eredeti útvonalakkal.

Futó megfigyelés közben is menthetsz. Ilyenkor a gomb megnyomásakor látható nézet rögzített pillanatképe kerül a fájlba; a későbbi tevékenységhez új export szükséges.

A mentés gomb akkor válik elérhetővé, ha az aktuális nézetben van eredmény. Az elrejtett adatok csak akkor kerülnek bele, ha exportálás előtt megjeleníted őket.

## CSV megnyitása táblázatkezelőben

A CSV táblázatot tartalmazó szövegfájl. Táblázatkezelőben és szövegszerkesztőben is megnyithatod.

Ha az Excel mindent egyetlen oszlopba tesz, használd a szöveg/CSV importálást, válassz **vessző** elválasztót és **UTF-8** kódolást. A fájl kódolásjelölőt is tartalmaz az ékezetes karakterek helyes megjelenítéséhez.

Az export teljes alkalmazás- és fájlútvonalakat tartalmaz. Megosztás előtt nézd át, ha a mappa- vagy fájlnevek személyes adatot tartalmazhatnak.

## Szabályok átvitele másik számítógépre

Az első számítógépen:

1. Nyisd meg a **Filter rules...** ablakot.
2. Add hozzá vagy frissítsd az átvinni kívánt szabályokat.
3. Válaszd az **Export...** lehetőséget, és mentsd el a `.tsv` fájlt.
4. Juttasd át a fájlt a másik számítógépre.

A második számítógépen:

1. Nyisd meg a VoidSentinelt, majd a **Filter rules...** ablakot.
2. Ha a jelenlegi listát biztonsági másolatként megőriznéd, először exportáld.
3. Válaszd az **Import (replace)...** lehetőséget, és nyisd meg az átvitt fájlt.
4. Ellenőrizd az útvonalakat és az opcionális alkalmazáskorlátozásokat. A környezeti változós útvonalak az aktuális felhasználóhoz igazodnak; a konkrét meghajtóútvonalakat szükséges lehet átírni.
5. A **Save rules** gombbal alkalmazd és mentsd el az importált listát.

A szabályfájl szűrési utasításokat tartalmaz. A rögzített eredmények és a figyelt EXE-k listája külön adatok.

## Mit jegyez meg az alkalmazás?

| Adat | Meddig marad meg? |
|---|---|
| A jelenlegi megfigyelés eredményei | A következő Start műveletig vagy a program bezárásáig; megőrzésükhöz ments CSV-t. |
| Figyelési lista | Alkalmazások hozzáadásakor vagy eltávolításakor mentődik. |
| Szűrési szabályok | A szerkesztő Save rules gombjával mentődnek. |
| Profil és kategóriakapcsolók | Az aktuális futáshoz válaszd ki őket; a szabályok alkotják az újrahasználható szűrési beállításokat. |

A figyelési lista és a szabályok a `%LOCALAPPDATA%\VoidSentinel` mappában találhatók. Megnyitásához másold az útvonalat a Fájlkezelő címsorába. A szokásos módosításokhoz használd a szabályszerkesztőt.

**Következő:** [Hibaelhárítás →](troubleshooting.md)
