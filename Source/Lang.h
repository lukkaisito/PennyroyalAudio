#pragma once
#include <JuceHeader.h>
#include <unordered_map>
#include <string>

// ============================================================
//  Lang  --  UI translations (English / Spanish)
//
//  Usage:  TR("English text")  ->  translated juce::String
//  Keys are the original English strings (ASCII only).
//  Spanish texts are UTF-8 (the project compiles with /utf-8 on MSVC).
// ============================================================
namespace Lang
{
    enum Id { English = 0, Spanish = 1 };

    inline int current = English;

    inline juce::StringArray names()
    {
        return { "English", juce::String::fromUTF8("Español") };
    }

    inline const std::unordered_map<std::string, const char*>& spanish()
    {
        static const std::unordered_map<std::string, const char*> m =
        {
            // ---- splash ----
            { "record  /  mix  /  distort",  "grabá  /  mezclá  /  distorsioná" },
            { "STARTING AUDIO ENGINE",       "INICIANDO MOTOR DE AUDIO" },
            { "LOADING TYPEFACES",           "CARGANDO TIPOGRAFÍAS" },
            { "BUILDING MIXER",              "ARMANDO EL MEZCLADOR" },
            { "READY",                       "LISTO" },
            { "EET N24 SIMON DE IRIONDO  /  OPEN SOURCE  /  GPLv3",
              "EET N24 SIMÓN DE IRIONDO  /  CÓDIGO ABIERTO  /  GPLv3" },

            // ---- top bar ----
            { "Open project  (Ctrl+O)",      "Abrir proyecto  (Ctrl+O)" },
            { "Save project  (Ctrl+S)",      "Guardar proyecto  (Ctrl+S)" },
            { "Undo  (Ctrl+Z)",              "Deshacer  (Ctrl+Z)" },
            { "Redo  (Ctrl+Y)",              "Rehacer  (Ctrl+Y)" },
            { "Quick-start tips",            "Guía rápida" },
            { "Stop  (back to start)",       "Detener  (vuelve al inicio)" },
            { "Play / Pause  (Space)",       "Reproducir / Pausa  (Espacio)" },
            { "Record on the armed track, or the selected one  (R)",
              "Grabar en la pista armada, o en la seleccionada  (R)" },
            { "Metronome",                   "Metrónomo" },
            { "Input monitor: hear your mic / guitar live",
              "Monitor de entrada: escuchá tu micrófono / guitarra en vivo" },
            { "Tempo: drag up / down",       "Tempo: arrastrá hacia arriba / abajo" },
            { "Master volume",               "Volumen general" },
            { "RECORDS ON",                  "GRABA EN" },
            { "  (armed)",                   "  (armada)" },

            // ---- quick start ----
            { "QUICK START",                 "GUÍA RÁPIDA" },
            { "Drop audio files on a track, or double-click a lane",
              "Soltá audios en una pista, o doble clic en un carril" },
            { "Press R to record on the selected (or armed) track",
              "Apretá R para grabar en la pista seleccionada (o armada)" },
            { "Drag clip edges to trim, top corners to fade",
              "Bordes del clip: recortar  /  esquinas: fades" },
            { "S split   /   Ctrl+D duplicate   /   Ctrl+Z undo",
              "S dividir   /   Ctrl+D duplicar   /   Ctrl+Z deshacer" },
            { "Don't show this again",       "No mostrar de nuevo" },
            { "GOT IT",                      "ENTENDIDO" },
            { "Close",                       "Cerrar" },

            // ---- track header ----
            { "Mute",                        "Silenciar" },
            { "Arm: record on this track",   "Armar: grabar en esta pista" },
            { "Volume (double-click: reset)","Volumen (doble clic: restablecer)" },
            { "Pan (double-click: centre)",  "Paneo (doble clic: centro)" },
            { "Rename",                      "Renombrar" },
            { "Import audio...",             "Importar audio..." },
            { "Delete track",                "Eliminar pista" },
            { "Delete \"%1\" and all its clips?", "¿Eliminar \"%1\" y todos sus clips?" },
            { "Delete",                      "Eliminar" },
            { "Cancel",                      "Cancelar" },

            // ---- arrangement ----
            { "RECORDING",                   "GRABANDO" },
            { "  [muted]",                   "  [silenciado]" },
            { "Rename clip",                 "Renombrar clip" },
            { "Split at playhead",           "Dividir en el cursor" },
            { "Duplicate",                   "Duplicar" },
            { "Unmute clip",                 "Activar clip" },
            { "Mute clip",                   "Silenciar clip" },
            { "Remove fades",                "Quitar fades" },
            { "Cut",                         "Cortar" },
            { "Copy",                        "Copiar" },
            { "Del",                         "Supr" },
            { "Import audio here...",        "Importar audio acá..." },
            { "Paste at playhead",           "Pegar en el cursor" },
            { "Set loop to this bar",        "Hacer loop de este compás" },
            { "Add audio track",             "Agregar pista de audio" },
            { "TRACKS  ",                    "PISTAS  " },
            { "Import audio",                "Importar audio" },
            { "Take ",                       "Toma " },

            // ---- mixer / effects ----
            { "On / off",                    "Encender / apagar" },
            { "EQ gain in dB (double-click: 0)", "Ganancia del EQ en dB (doble clic: 0)" },
            { "Pan",                         "Paneo" },
            { "Compressor",                  "Compresor" },
            { "HI",                          "AGU" },
            { "MID",                         "MED" },
            { "LO",                          "GRA" },
            { "COMPRESSOR",                  "COMPRESOR" },
            { "Tone",                        "Tono" },
            { "Level",                       "Nivel" },
            { "Thresh",                      "Umbral" },
            { "Makeup",                      "Compens." },
            { "Time",                        "Tiempo" },
            { "Feedback",                    "Repetición" },
            { "Mix",                         "Mezcla" },
            { "Size",                        "Tamaño" },
            { "Damp",                        "Amortig." },
            { "MIXER",                       "MEZCLADOR" },
            { "EQ, effects and levels per track", "EQ, efectos y niveles por pista" },
            { "DEVICES  /  ",                "EFECTOS  /  " },

            // ---- plugins ----
            { "PLUGINS",                     "PLUGINS" },
            { "VST3 scanner",                "Buscador de VST3" },
            { "SCAN VST3 FOLDER",            "BUSCAR CARPETA VST3" },
            { "LOAD SELECTED",               "CARGAR SELECCIONADO" },
            { "No plugins scanned yet. Choose the folder where your VST3 plugins are installed.",
              "Todavía no se buscaron plugins. Elegí la carpeta donde tenés instalados tus VST3." },
            { "Select VST3 folder",          "Elegí la carpeta de VST3" },
            { "Scanning...",                 "Buscando..." },
            { "Found %1 plugin(s).",         "Se encontraron %1 plugin(s)." },
            { "Select a plugin first.",      "Primero elegí un plugin." },
            { "\"%1\" selected. Full plugin hosting is planned for a future version; use the built-in effects in the MIXER meanwhile.",
              "\"%1\" seleccionado. La carga completa de plugins está planeada para una próxima versión; mientras tanto usá los efectos del MEZCLADOR." },

            // ---- export ----
            { "EXPORT",                      "EXPORTAR" },
            { "Render your project",         "Renderizá tu proyecto" },
            { "MIXDOWN",                     "MEZCLA FINAL" },
            { "EXPORT WAV",                  "EXPORTAR WAV" },
            { "EXPORT MP3",                  "EXPORTAR MP3" },
            { "Export WAV",                  "Exportar WAV" },
            { "Rendering...",                "Renderizando..." },
            { "Exported: ",                  "Exportado: " },
            { "Export failed.",              "Falló la exportación." },
            { "MP3 export",                  "Exportar MP3" },
            { "MP3 export needs the LAME encoder, which is not included in this build.\nExport as WAV and convert it with any free converter.",
              "Exportar a MP3 necesita el codificador LAME, que no viene incluido en esta versión.\nExportá en WAV y convertilo con cualquier conversor gratuito." },
            { "Renders every track with its EQ, effects, volume and pan to a 24-bit stereo WAV. Includes 2 seconds of tail for reverb and delay.",
              "Renderiza todas las pistas con su EQ, efectos, volumen y paneo a un WAV estéreo de 24 bits. Incluye 2 segundos de cola para la reverb y el delay." },

            // ---- settings ----
            { "SETTINGS",                    "AJUSTES" },
            { "Appearance, language, audio device and latency",
              "Apariencia, idioma, dispositivo de audio y latencia" },
            { "Language",                    "Idioma" },
            { "Theme",                       "Tema" },
            { "Outline colour",              "Color de contornos" },
            { "Pick any colour",             "Elegí cualquier color" },
            { "CUSTOM...",                   "PERSONALIZAR..." },
            { "APPLY",                       "APLICAR" },
            { "Buffer size",                 "Tamaño de buffer" },
            { " samples",                    " muestras" },
            { "Help",                        "Ayuda" },
            { "Show quick-start tips when the program opens",
              "Mostrar la guía rápida al abrir el programa" },

            // ---- tabs ----
            { "ARRANGE",                     "ARREGLO" },
            { "TUNER",                       "AFINADOR" },
            { "CHROMATIC TUNER",             "AFINADOR CROMÁTICO" },

            // ---- status / files ----
            { "SPACE play   R record (armed or selected track)   S split   CTRL+D duplicate   CTRL+Z undo   ALT+drag: no snap   CTRL+wheel: zoom",
              "ESPACIO reproducir   R grabar (pista armada o seleccionada)   S dividir   CTRL+D duplicar   CTRL+Z deshacer   ALT+arrastrar: sin imán   CTRL+rueda: zoom" },
            { "Theme: ",                     "Tema: " },
            { "Language: ",                  "Idioma: " },
            { "Clip copied",                 "Clip copiado" },
            { "Saved ",                      "Guardado: " },
            { "Save failed",                 "No se pudo guardar" },
            { "Could not write ",            "No se pudo escribir " },
            { "Save project",                "Guardar proyecto" },
            { "Open project",                "Abrir proyecto" },
            { "Opened ",                     "Abierto: " },
            { "Open failed",                 "No se pudo abrir" },
            { "This file is not a valid Pennyroyal project.",
              "Este archivo no es un proyecto válido de Pennyroyal." },
            { "Audio Device Error",          "Error del dispositivo de audio" },
        };
        return m;
    }

    inline juce::String tr(const char* en)
    {
        if (current == Spanish)
        {
            const auto& m = spanish();
            auto it = m.find(en);
            if (it != m.end())
                return juce::String::fromUTF8(it->second);
        }
        return juce::String(en);
    }
}

inline juce::String TR(const char* english) { return Lang::tr(english); }
