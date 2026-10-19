# OpenCraft — Soporte y Compilación en Android

Este directorio contiene la configuración del proyecto Android con integración de **Android NDK** y **SDL3**.

## Requisitos de Entorno

- **Android SDK:** API 24 (Android 7.0 Nougat) como mínimo, API 34 (Android 14) como destino.
- **Android NDK:** r25c o superior con soporte C++20 (`c++_shared`).
- **Arquitecturas:** `arm64-v8a` (principal), `armeabi-v7a`, `x86_64` (emuladores).
- **Backend Gráfico:** OpenGL ES 3.0 nativo.

## Procedimiento de Construcción del APK

```bash
# 1. Asegurar SDK y NDK exportados
export ANDROID_HOME=/ruta/al/android/sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/25.2.9519653

# 2. Compilar con Gradle
cd platforms/android
./gradlew assembleDebug

# 3. Instalar en dispositivo conectado vía ADB
adb install -r build/outputs/apk/debug/android-debug.apk
```

## Características Verificadas en Fase 14

- Ciclo de vida completo: `SDL_EVENT_WILL_ENTER_BACKGROUND` guarda el mundo y silencia el audio inmediatamente.
- Soporte para display cutout / notch en pantallas panorámicas modernas ($> 18:9$).
- Controles multitáctiles con joystick virtual, botones de acción simultáneos y Hotbar adaptativa.
- Descubrimiento LAN sobre Wi-Fi mediante multicast UDP (puerto 35566).
