# Hibaelhárítás

[Kézikönyv főoldala](README.md) · [English](../en/troubleshooting.md)

## Üres az eredménylista

Ellenőrizd az alábbiakat ebben a sorrendben:

1. A megfelelő alkalmazás-EXE-t adtad hozzá, és megnyomtad a **Start** gombot?
2. Végezz el fájlt módosító feladatot, például ments dokumentumot. Egy fájl puszta olvasása nem tartozik a megjelenített változtatási műveletekhez.
3. Ha az alkalmazás segédprogramokat használ, indítás előtt kapcsold be az **Include child processes** lehetőséget.
4. Válaszd a **Full application footprint** profilt a további kategóriák megjelenítéséhez.
5. Ellenőrizd a **Filter rules...** ablakban, hogy nem érinti-e a helyet egy Exclude szabály.

Egyes parancsikonok indítóprogramot futtatnak, vagy egy már futó, különálló alkalmazást kérnek meg a feladatra. Ha a tényleges író a figyelt folyamatfán kívül van, add hozzá annak EXE-jét is. A megfigyelés elindítása nem állítja helyre a korábban történt tevékenységet.

## Túl sok a háttértevékenység

Először használd a **Meaningful changes** profilt. A Background, Metadata, Errors, Unconfirmed és Transient kapcsolókat csak akkor kapcsold be, ha szükséged van rájuk.

Ismétlődően zajos hely esetén kattints jobb gombbal az eredményre, és készíts mappaszabályt. **Background** műveletet válassz, ha kényelmes kapcsolóval szeretnéd megjeleníteni vagy elrejteni; **Exclude** műveletet, ha mindkét profilban elrejtenéd. A szabályokat célszerű konkrét almappákra korlátozni, hogy az alkalmazás többi beállítása látható maradjon.

## Egy fájl továbbra is hiányzik az egyszerű nézetből

Kapcsold be a kategóriákat, vagy válts a teljes profilra. Az útvonal háttértevékenységként, átmeneti jelöltként vagy feloldatlanként szerepelhet. Az Include felülírhatja az útvonalkizárásokat; a sikertelen és nem megerősített kimenetelekhez továbbra is szükség van a megfelelő kapcsolóra.

## Nem indul a megfigyelés

- A normál EXE megnyitásakor hagyd jóvá a rendszergazdai engedélykérést.
- Ha egy másik VoidSentinel-példány már figyel, először ott kattints a **Stop** gombra. Egyszerre egy példány rögzíthet.
- Olvasd el az állapotsor hibaüzenetét. Felügyelt számítógépen, ha a Windows megakadályozza a megfigyelést, fordulj a rendszergazdához.

Kényszerített programbezárás után egy későbbi Start helyreállíthatja a VoidSentinel hátramaradt megfigyelési munkamenetét.

## A Lost vagy Dropped értéke nagyobb nullánál

Egyes események kimaradtak, ezért a rögzítés hiányos. Ha hasznos, mentsd el, majd ismételj meg egy kisebb feladatot rövidebb munkamenetben, lehetőség szerint kevesebb egyéb lemeztevékenység mellett.

A láthatósági szűrők a nézetet módosítják; a Windows mögöttes eseményfolyamát nem csökkentik. A hiányzó eseményeket utólagos nézetváltással sem lehet visszaállítani.

## Ismeretlen típus vagy feloldatlan útvonal

Az **Unknown** azt jelenti, hogy a fájl/mappa típusa nem volt megállapítható. Az `[unresolved file object ...]` bejegyzésnél az útvonalat nem sikerült hozzárendelni. Vizsgálatukhoz kapcsold be az **Unconfirmed** lehetőséget, vagy használd a teljes profilt.

Ezek a címkék az elérhető eseményadatok hiányosságait jelzik. Nem azonosak egy Windows-kérés sikertelenségével.

## Sikertelen mentés vagy importálás

- Exporthoz válassz írható helyet.
- Ha másik alkalmazás zárolja a célfájlt, zárd be abban, majd próbáld újra a mentést.
- Az **Export...** gombbal létrehozott `.tsv` szabályfájlt importáld, ne az eredményeket tartalmazó CSV-t.
- Átvitel után ellenőrizd a környezeti változókat és a teljes útvonalakat.

Érvénytelen import esetén a jelenlegi lista megmarad. A sikertelen fájlcsere úgy van kialakítva, hogy megtartsa a korábbi célfájlt. A szabályszerkesztő megjeleníti a saját hibaüzenetét; az eredményexport hibája az állapotsorban látható.

## Réginek látszik az ikon vagy a verzió

Fordítás után a legújabb `dist\VoidSentinel.exe` fájlt használd. Frissítsd a mappanézetet a Fájlkezelőben vagy a Total Commanderben. A régi buildmappák korábbi fejlesztési példányokat tartalmazhatnak.

## Mit írj egy hibajelentésbe?

Add meg a Windows verzióját, a figyelt alkalmazást, a kiválasztott profilt és kapcsolókat, az érintett szabályokat, a reprodukálás lépéseit és az esetleges Lost/Dropped értékeket. Ha hasznos, mellékelj egy kisebb CSV-t vagy képernyőképet, miután átnézted a személyes útvonalakat és fájlneveket.

**Következő:** [Fordítás forráskódból →](build.md)
