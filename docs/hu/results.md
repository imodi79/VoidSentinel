# Az eredmények értelmezése

[Kézikönyv főoldala](README.md) · [English](../en/results.md)

## Kezdd a Meaningful changes nézettel

Ez a profil arra válaszol: **Mely fájlokat és mappákat érintette a figyelt alkalmazás?** Az azonos útvonalhoz tartozó műveleteket a figyelt alkalmazás alatt egy összefoglalóba vonja össze.

Például egy kitalált alkalmazás segédprogramot indíthat egy jelentés mentéséhez:

| Alkalmazás | Útvonal | Változás | Folyamatlánc |
|---|---|---|---|
| ExampleApp.exe | `C:\ExampleOutput\report.txt` | Created; Written / resized | ExampleApp.exe → ExampleHelper.exe |

A folyamatlánc a közreműködő alkalmazások megfigyelt kapcsolatát mutatja. A segédalkalmazásra a *gyermekfolyamat* kifejezést is használjuk. Ha a gyermekfolyamat EXE-jét külön hozzáadod a figyelési listához, saját alkalmazáscsoportot kap.

Az ismételt írások új sorok helyett a számlálót növelik. A fájlírásokat és a méretváltoztatásokat a **Written / resized** foglalja össze. Ha a program ideiglenes fájlon keresztül ment, majd azt átnevezi, az összefoglaló megmutathatja a végleges célútvonalat, az eredeti útvonal pedig megmarad a CSV-ben.

## Az oszlopok jelentése

| Oszlop | Jelentés |
|---|---|
| Application | Összefoglalóban a figyelt alkalmazás; technikai sorokban a műveletet végrehajtó alkalmazás. |
| Type | Folder: mappa; File: fájl; Unknown: a Windows nem adott elegendő információt a típushoz. |
| Path | Az érintett teljes elérési út. Összefoglalóban a végleges átnevezési cél is szerepelhet. |
| Change / operation | A közérthető összefoglaló vagy az egyedi technikai művelet. |
| Count | A megjelenítésbe bevont fájlműveleti kérések száma. Tíz kérés nem feltétlenül tíz felhasználói művelet. |
| First seen / Last seen | A bejegyzés rögzített időtartománya, helyi idő szerint. |
| Result | Siker, vegyes eredmény vagy egy kérés kimenetele. |
| Process chain | A figyelt alkalmazás és a megfigyelt segédprogramlánc. Több közreműködő is szerepelhet. |
| Rename destination | Az átnevezés célja, ha hozzá lehetett rendelni a kéréshez. |

A további oszlopokhoz görgess vízszintesen. Dupla kattintással megnyithatod a bejegyzés részleteit. A CSV a teljes alkalmazásútvonalakat, valamint a sikeres, sikertelen és nem megerősített kérések külön számlálóit is tartalmazza.

Az alkalmazáscsoportokon belül a mappák megelőzik a fájlokat, és mindkét rész teljes útvonal szerint rendezett. Az **Unknown** bejegyzések kerülnek a végére.

## Mikor használd a Full application footprint nézetet?

Ezt a profilt válaszd a beállítások, gyorsítótárak, ideiglenes fájlok vagy sikertelen kérések vizsgálatához. Bekapcsolja a technikai sorokat és az összes kategóriakapcsolót.

A saját **Exclude** szabályaid továbbra is érvényesek. Kapcsold ki vagy módosítsd az adott szabályt, ha a vele elrejtett tevékenységet is látni szeretnéd.

## Kategóriakapcsolók

| Kapcsoló | Ezt jeleníti meg |
|---|---|
| Technical rows | Fájlonkénti összefoglalók helyett külön alkalmazás/útvonal/művelet bejegyzéseket. |
| Errors | Sikertelen kéréseket. |
| Unconfirmed | Függőben maradt vagy hiányos kéréseket, valamint fel nem oldott fájlútvonalakat. |
| Background | A szabályok által háttértevékenységnek minősített útvonalakat. |
| Transient | Olyan jelölteket, amelyek létrehozását és későbbi törlési kérését is megfigyeltük a munkamenetben. |
| Metadata | Attribútumokat, időbélyegeket és más fájlinformációkat érintő változtatásokat. |

A kapcsolókat rögzítés közben és utána is átállíthatod. Az elrejtett bejegyzések a következő **Start** műveletig vagy az alkalmazás bezárásáig elérhetők maradnak.

## Figyeld az állapotsort is

A **Rows** a látható eredménysorok száma; az **Events** a rögzített mögöttes kéréseket számolja. A **Hidden** számlálók megmutatják, milyen okból maradtak ki kérések az aktuális nézetből. Kéréseket számolnak, ezért értékük nagyobb lehet a látható sorok számánál.

Ha a **Lost** vagy **Dropped** értéke nagyobb nullánál, a rögzítés hiányos. Lásd: [Hibaelhárítás](troubleshooting.md).

A **Success** egy Windows-kérés sikerét jelenti. A **Deletion requested** önmagában nem bizonyítja, hogy a fájl már eltűnt: másik program még nyitva tarthatja. A **Transient candidate** a megfigyelt kérések alapján adott besorolás. A pontosság részleteit a [Műszaki háttér](technical-reference.md) ismerteti.

**Következő:** [Szűrési szabályok →](filters.md)
