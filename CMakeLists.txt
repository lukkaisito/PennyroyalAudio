cmake_minimum_required(VERSION 3.22)
project(PennyroyalAudio VERSION 1.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------
# Windows: runtime de MSVC estatico.
# El .exe ya NO necesita vc_redist ni MSVCP140.dll / VCRUNTIME140.dll
# ---------------------------------------------------------------
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ---------------------------------------------------------------
# 1. JUCE
#    - Si JUCE_DIR apunta a una copia local valida, se usa esa.
#    - Si no, se descarga JUCE 8.0.12 automaticamente (GitHub / PC nueva).
# ---------------------------------------------------------------
set(JUCE_DIR "" CACHE PATH "Ruta opcional a una copia local de JUCE 8")

if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Usando JUCE local: ${JUCE_DIR}")
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    message(STATUS "JUCE local no encontrado -> descargando JUCE 8.0.12")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------
# 2. App
# ---------------------------------------------------------------
juce_add_gui_app(PennyroyalAudio
    PRODUCT_NAME        "Pennyroyal Audio"
    VERSION             "${PROJECT_VERSION}"
    COMPANY_NAME        "Pennyroyal Studio"
    BUNDLE_ID           "com.pennyroyalstudio.daw"
    NEEDS_MIDI_INPUT    TRUE
    NEEDS_MIDI_OUTPUT   TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Pennyroyal Audio needs microphone access."
)

juce_generate_juce_header(PennyroyalAudio)

# Embedded fonts (Resources/Fonts) -> BinaryData::...
file(GLOB FontFiles CONFIGURE_DEPENDS "Resources/Fonts/*.ttf")
juce_add_binary_data(PennyroyalAssets SOURCES ${FontFiles})
set_target_properties(PennyroyalAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# ---------------------------------------------------------------
# 3. Fuentes (todo lo que este en Source/)
# ---------------------------------------------------------------
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "Source/*.cpp" "Source/*.h")
target_sources(PennyroyalAudio PRIVATE ${SourceFiles})

# ---------------------------------------------------------------
# 4. Definiciones
# ---------------------------------------------------------------
target_compile_definitions(PennyroyalAudio PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_AU=0
)

# MSVC: read the source files as UTF-8 (Spanish texts with accents)
if(MSVC)
    target_compile_options(PennyroyalAudio PRIVATE /utf-8)
endif()

# ---------------------------------------------------------------
# 5. Modulos JUCE
# ---------------------------------------------------------------
target_link_libraries(PennyroyalAudio
    PRIVATE
        PennyroyalAssets
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------
# 6. Instalacion + paquetes Linux (.deb y .tar.gz) con CPack
#    El instalador de Windows se hace con Inno Setup (packaging/windows)
# ---------------------------------------------------------------
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    install(PROGRAMS $<TARGET_FILE:PennyroyalAudio>
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            RENAME pennyroyal-audio)
    install(FILES packaging/linux/pennyroyal-audio.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/pennyroyal-audio.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

    set(CPACK_GENERATOR                    "DEB;TGZ")
    set(CPACK_PACKAGE_NAME                 "pennyroyal-audio")
    set(CPACK_PACKAGE_VERSION              "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_VENDOR               "Pennyroyal Studio")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY  "Open-source Digital Audio Workstation built with JUCE")
    set(CPACK_PACKAGING_INSTALL_PREFIX     "/usr")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER    "Pennyroyal Studio")
    set(CPACK_DEBIAN_PACKAGE_SECTION       "sound")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS     ON)
    # Nombres fijos (sin numero de version) para que los links de la web no cambien
    set(CPACK_DEBIAN_FILE_NAME             "pennyroyal-audio_amd64.deb")
    set(CPACK_ARCHIVE_FILE_NAME            "pennyroyal-audio-linux")
    include(CPack)
endif()
