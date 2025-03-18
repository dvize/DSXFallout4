# Buffout4 VR

It's like buffout but for the engine. This is a fork for VR.

* [Fallout4](https://www.nexusmods.com/fallout4/mods/47359)
* [Fallout4 VR](https://www.nexusmods.com/fallout4/mods/64880)

# Build Dependencies
* [AutoTOML](https://github.com/Ryan-rsm-McKenzie/AutoTOML)
* [Boost](https://www.boost.org/)
	* Algorithm
	* Container
	* Nowide
	* Stacktrace
* [CommonLibF4](https://github.com/Ryan-rsm-McKenzie/CommonLibF4)
* [CommonLibF4 VR enabled](https://github.com/alandtse/CommonLibF4)
	* Add this as as an environment variable `CommonLibF4Path` or use the submodule in /external
* [fmt](https://github.com/fmtlib/fmt)
* [Frozen](https://github.com/serge-sans-paille/frozen)
* [infoware](https://github.com/ThePhD/infoware)
* [mmio](https://github.com/Ryan-rsm-McKenzie/mmio)
* [rsm-binary-io](https://github.com/Ryan-rsm-McKenzie/binary_io)
* [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)
* [spdlog](https://github.com/gabime/spdlog)
* [TBB](https://github.com/oneapi-src/oneTBB)
* [Xbyak](https://github.com/herumi/xbyak)

# End User Dependencies
* [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327)
* [VR Address Library for F4SEVR](https://www.nexusmods.com/fallout4/mods/64879)
* [F4SE](https://f4se.silverlock.org/)
* [F4SEVR](https://f4se.silverlock.org/)
* [Microsoft Visual C++ Redistributable for Visual Studio 2019](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads)
* [TBB](https://github.com/oneapi-src/oneTBB/releases/latest)

# Building
```
git clone https://github.com/alandtse/Buffout4.git
cd Buffout4
# pull commonlib /external to override the path settings
git submodule init
# to update submodules to checked in build
git submodule update
```

### Fallout 4
```
cmake --preset vs2022-windows-vcpkg
cmake --build build --config Release
```
### VR
```
cmake --preset vs2022-windows-vcpkg-vr
cmake --build buildvr --config Release
```

# Credits
* [Ryan-rsm-Mckenzie](https://github.com/Ryan-rsm-McKenzie) - Original code and framework and for enabling the commonlib community in Skyrim and Fallout
* PDB requires `msdia140.dll` distributed under [Visual Studio C++ Redistributable](https://docs.microsoft.com/en-us/visualstudio/releases/2022/redistribution#dia-sdk). [PDB Handler](src/Crash/PDB/PdbHandler.cpp) derived from StackOverflow code.

# License
[MIT](LICENSE)
