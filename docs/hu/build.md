# Fordítás forráskódból

[Kézikönyv főoldala](README.md) · [English](../en/build.md)

A fordítás a projekt forrásfájljaiból futtatható alkalmazást készít. Ha már megvan a lefordított EXE, folytasd a [Gyors kezdés](quick-start.md) oldallal.

## A fejlesztőeszközök telepítése

Windows 11 x64 rendszert használj. Telepítsd a **Visual Studio Community** kiadást, és a telepítőben válaszd a **Desktop development with C++** csomagot. Legyen benne:

- MSVC C++ build tools for x64/x86.
- Windows 11 SDK.
- C++ CMake tools for Windows.

A Git hasznos a munkapéldány letöltéséhez és frissítéséhez. Letöltött forrásarchívummal is dolgozhatsz.

## Az alkalmazás fordítása

1. Nyisd meg a `CMakeLists.txt` és `build.ps1` fájlokat tartalmazó projektmappát.
2. Nyiss PowerShellt ebben a mappában. A Fájlkezelő helyi menüjében elérhető lehet az **Open in Terminal**, illetve **Megnyitás a Terminálban** lehetőség.
3. Futtasd:

```powershell
.\build.ps1
```

Várd meg a sikeres befejezést. A szkript megkeresi a telepített Visual Studio-eszközöket, lefordítja az alkalmazást, és ide másolja a hordozható EXE-t:

```text
dist\VoidSentinel.exe
```

A fordító kimenete az `out\Release\VoidSentinel.exe` helyen is megtalálható. Futtatáshoz vagy az alkalmazás másolásához a `dist` példány legyen az egyértelmű kiindulópont.

Az EXE tartalmazza az ikonjait és a C/C++ futtatókönyvtárat. Átmásolhatod másik kompatibilis Windows 11 x64 gépre; azon a futtatáshoz nem szükségesek a fejlesztőeszközök.

## Hibakeresési változat fordítása

A problémát vizsgáló fejlesztők ezt futtathatják:

```powershell
.\build.ps1 -Configuration Debug
```

Ez is frissíti a `dist` példányt, ezúttal a Debug buildből. Az alapértelmezett paranccsal fordíts újra, ha ismét Release konfigurációt szeretnél. A „Release konfiguráció” itt fordítási beállítás; a GitHub-kiadás közzététele külön lépés.

## Ha sikertelen a fordítás

Olvasd el a terminál utolsó hibaüzenetét. Hiányzó eszköz esetén általában a fenti összetevők valamelyikét kell hozzáadni a Visual Studio Installerben. Ha a cél-EXE fut, újrafordítás előtt zárd be azt a példányt, hogy a build felül tudja írni.

Olyan PowerShell-munkamenetet használj, amelyben engedélyezett a projekt helyi buildszkriptjének futtatása. Felügyelt gépen kövesd a rendszergazda szkriptfuttatási szabályait.

A fejlesztői ellenőrzőparancsokat a [Műszaki háttér](technical-reference.md) ismerteti.

**Következő:** [Gyors kezdés →](quick-start.md)
