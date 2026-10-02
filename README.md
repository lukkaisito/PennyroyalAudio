# Pennyroyal Audio

**DAW (Digital Audio Workstation) open-source hecho en C++17 con JUCE 8.**
Proyecto Final — EET N°24 "Simón de Iriondo", Resistencia, Chaco, Argentina.

Pennyroyal Audio es una estación de trabajo de audio gratuita y de código abierto,
pensada para músicos que no pueden (o no quieren) pagar licencias de DAWs comerciales.
Forma parte de un ecosistema que incluye un pedal de Overdrive analógico diseñado a medida.

## Funcionalidades

- Reproducción y grabación multipista
- Importación de audio (WAV, MP3, AIFF, FLAC, OGG)
- EQ de 3 bandas por pista (filtros biquad propios)
- Loop de reproducción con puntos IN / OUT arrastrables
- Edición de clips: cortar, copiar, pegar y borrar
- Nombres editables de pistas y clips
- Zoom en el timeline
- Hosting básico de plugins VST3
- Afinador cromático (algoritmo NSDF, A4 = 440 Hz)
- Metrónomo, Input Monitor, control de buffer/latencia
- Guardar / abrir proyectos (`.pennyr`)
- Exportación a WAV / MP3
  
## Descarga

Los instaladores están en la sección **[Releases](../../releases)**:

| Sistema | Archivo |
|---|---|
| Windows 10/11 (64 bits) | `PennyRoyalAudioSetup.exe` |
| Cualquier Linux (Ubuntu, Fedora, Mint...) | `PennyRoyalAudio-x86_64.AppImage` |
| Ubuntu / Debian / Mint | `pennyroyal-audio_amd64.deb` |
| Otras distros (portable) | `pennyroyal-audio-linux.tar.gz` |

AppImage: darle permiso de ejecución (`chmod +x PennyRoyalAudio-x86_64.AppImage`) y abrirlo.
.deb: `sudo apt install ./pennyroyal-audio_amd64.deb`

## Compilar desde el código

**Requisitos:** CMake 3.22+, un compilador C++17 y Git.
JUCE se descarga solo la primera vez que se configura el proyecto.

### Windows (CLion o Visual Studio)
1. Abrir la carpeta del proyecto en CLion.
2. Seleccionar el perfil **Release** y compilar (`Ctrl+F9`).
3. Instalador: abrir `packaging/windows/PennyroyalAudio.iss` con
   [Inno Setup](https://jrsoftware.org/isinfo.php) y presionar *Compile*.

### Linux
```bash
bash packaging/linux/build-linux.sh
```

## Tecnologías

C++17 · JUCE 8 · CMake · Inno Setup · CPack · GitHub Actions

## Licencia

Este proyecto usa JUCE bajo su licencia open-source (AGPLv3),
por lo que el código de Pennyroyal Audio se distribuye bajo **AGPLv3**.
